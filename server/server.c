#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"
#include "user.h"

/* serve offers a variety of services to
 * the client. It handles login, registration,
 * session creation, session joining, session
 * leaving, session querying, session user
 * querying, session messaging, session kicking,
 * and session promoting. It deals with multiple
 * client connections with the use of threads.*/

user_database* user_list;
pthread_mutex_t rw_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t user_list_mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char* argv[]){
    if (argc != 2){
        fprintf(stderr, "Usage: server <port>\n");
        exit(1);
    }
    //cited from Page 21 and Page 28, Beej's Guide to Network Programming, with modifications
    int status;
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    struct addrinfo hints;
    struct addrinfo *servinfo, *p; // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        freeaddrinfo(servinfo); // free the linked-list
        exit(1);
    }
    printf("getaddrinfo success\n");

    int listen_fd;

    // This code is cited from Page 26 of Beej's Guide to Network Programming.
    // It sets the option on the socket to allow the reuse of the local address in case
    // the server program is restarted before the old socket times out.
    int yes=1;
    // char yes='1'; // Solaris people use this
    // adapted from Page 28 of Beej's Guide to Network Programming.
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((listen_fd = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("setsockopt"); // If setting the option fails, print an error received_message.
            exit(1); // Exit the program with an error code.
        }
        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_fd);
            perror("client: connect");
            continue;
        }
        break;
    }
    // end of adaptation


    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }
    printf("bind success\n");

    user_list = user_database_init();
    if (user_list == NULL){
        perror("user_database_init");
        exit(errno);
    }

    if (set_up_database() != 1){
        perror("init_database");
        exit(errno);
    }

    freeaddrinfo(servinfo); // all done with this structure
    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(errno);
    }
    printf("listen success\n");
    pthread_t thread_pool[THREAD_CAPACITY];
    int thread_count = 0;

    while (1) {
        // check if thread pool is full
        // if full, wait for all threads to finish before exiting
        // and clean up their resources
        if (thread_count == THREAD_CAPACITY) {
            printf("thread pool full\n");
            for (int i = 0; i < thread_count; i++){
                pthread_join(thread_pool[i], NULL);
            }
            break;
        }
        puts("accepting connections...");
        int accept_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &addr_size);
        if (accept_fd == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            else{
                perror("accept");
                exit(errno);
            }
        }
        pthread_create(&thread_pool[thread_count], NULL, &connection_handler, (void *)&accept_fd);
        thread_count++;
    }
    //wait for other threads to finish before exiting
    remove_all_users_in_database(user_list, &user_list_mutex);
    return 1;
}

/* handle each connection in a separate thread
 each thread will be responsible for one client*/
void* connection_handler(void* accept_fd){
    // initialize variables
    // and get file descriptor of the socket
    int socket_file_descriptor = *(int *)accept_fd;
    message_t received_message, reply_message;
    char buffer[maximum_buffer_size];
    char source[MAX_NAME];
    char server_connection_status = OFFLINE;

    while(1){
        // memset to avoid garbage values
        memset(buffer, 0, sizeof(buffer));
        memset(&received_message, 0, sizeof(received_message));
        memset(&reply_message, 0, sizeof(reply_message));
        // receive message from client and error check
        size_t bytes_received = receive_message(&socket_file_descriptor, buffer);

        if (bytes_received == CONNECTION_REFUSED){
            if (server_connection_status == ONLINE){
                printf("user %s disconnected\n", source);
                server_side_user_exit(source);
            }
            break;
        }
        if (bytes_received == RECEIVE_ERROR){
            if (server_connection_status == ONLINE){
                printf("came across abnormality\n");
                server_side_user_exit(source);
            }
            perror("receive failed");
            break;
        }
        // convert received message to a message_t struct
        // and type check
        string_to_message(&received_message, buffer);

        // if message type is login, check if the user is already logged into
        // the same account if not, verify the login information.
        // Note that login to a different account whilr connected
        // is hanled in the client side
        if (received_message.type == LOGIN){
            int reply_type = verify_login(received_message.source,received_message.data);

            if (reply_type == SUCCESS_LOGIN)
            {
                fprintf(stdout,"============================\n");
                fprintf(stdout, "Verification Success!\n");
                server_connection_status = ONLINE;
                user_login((char *)received_message.source, ONLINE, &socket_file_descriptor);
                strcpy(source, (char*)received_message.source);
                fprintf(stdout, "user %s just landed!!\n", source);
                fprintf(stdout,"============================\n");
                fill_message(&reply_message,LO_ACK,0, (char *)received_message.source, NULL);
            }
            else{
                fill_message(&reply_message, LO_NAK,
                             strlen(select_login_message(reply_type)),
                             (char *)received_message.source, select_login_message(reply_type));
            }
            message_to_string(&reply_message, reply_message.size, buffer);
        }

        // if message type is create, check if the user is already logged in
        // if not, verify the registration information. If the registration
        // name is already taken, send a NAK message to the client
        if (received_message.type == CREATE){
            int reply_type = user_registration((char *)received_message.source,
                                               (char *)received_message.data,
                                               &socket_file_descriptor);
            if (reply_type == ADD_USER_SUCCESS)
            {
                fprintf(stdout,"============================\n");
                fprintf(stdout, "Registration Success!\n");
                server_connection_status = ONLINE;
                strcpy(source, (char*)received_message.source);
                fprintf(stdout, "user %s just landed!!\n", source);
                fprintf(stdout,"============================\n");
                fill_message(&reply_message,RG_ACK,0, (char *)received_message.source, NULL);
            }
            else{
                fill_message(&reply_message, RG_NAK,
                             strlen(user_registration_message(reply_type)),
                             (char *)received_message.source, user_registration_message(reply_type));
            }
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        // exit current user and reset their user information
        // name and password are not reset
        if (received_message.type == EXIT){
            server_side_user_exit(source);
            break;
        }
        // join user to a session and do some edge case checking
        if (received_message.type == JOIN){
            server_side_session_join(source, (char*)received_message.data);
            continue;
        }
        // create a new session and do some edge case checking
        if (received_message.type == NEW_SESS){
            server_side_session_create(source, (char*)received_message.data);
            continue;
        }
        // leave a session
        if (received_message.type == LEAVE_SESS){
            server_side_session_leave(source);
            continue;
        }
        // send the active user list to the client
        if (received_message.type == QUERY){
            char list[maximum_buffer_size];
            memset(list, 0, sizeof(list));
            get_active_user_list_in_database(user_list, list, &rw_mutex);
            fill_message(&reply_message, QU_ACK,
                         strlen(list), (char *)received_message.source, list);
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        // send the active user list for a session to the client
        // and do some edge case checking
        if (received_message.type == SESS_QUERY){
            char list[maximum_buffer_size];
            memset(list, 0, sizeof(list));
            int code = get_active_user_list_in_a_session(user_list,list,
                                            (char*)received_message.data, &rw_mutex);
            if (code == SESSION_NAME_INVALID)
                fill_message(&reply_message, SQ_NAK,
                             strlen(session_response_message(code)),
                             (char *)received_message.source,
                             session_response_message(code));
            else
                fill_message(&reply_message, SQ_ACK,
                         strlen(list), (char *)received_message.source, list);
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        // send a group message in a session
        if (received_message.type == MESSAGE){
            fill_message(&reply_message, MESSAGE,
                         received_message.size,
                         (char *)received_message.source, (char*)received_message.data);
            user_t* sending_user = return_user_by_username(user_list, received_message.source);
            send_message_in_a_session(&reply_message, sending_user->session_id);
            continue;
        }

        // kick a user from a session
        // and do some edge case checking
        if (received_message.type == KICK){
            server_side_kick_user(source, (char*)received_message.data);
            continue;
        }

        // promote a user to be the owner of a session
        // and do some edge case checking
        if (received_message.type == PROMOTE){
            server_side_promote_user(source, (char*)received_message.data);
            continue;
        }

        // send the message to the client
        // and error check
        size_t bytes_sent = send_message(&socket_file_descriptor,strlen(buffer),buffer);
        if (bytes_sent == CONNECTION_REFUSED){
            // if client is closed abruptly, exit the
            // user to clear their statuses
            if (server_connection_status == ONLINE){
                server_side_user_exit(source);
            }
            break;
        }
        if (bytes_sent == SEND_ERROR){
            perror("receive failed");
            // if client is closed abruptly, exit the
            // user to clear their statuses
            if (server_connection_status == ONLINE){
                server_side_user_exit(source);
            }
            break;
        }

    }
    close(socket_file_descriptor);
    fprintf(stdout, "thread exited\n");
    return NULL;
}

/*   verify login information
     returns LIST_EMPTY if user_list is not initialized
     returns USERNAME_ERROR if user does not exist
     returns PASSWORD_ERROR if password does not match.
     returns ALREADY_LOGIN if user is already logged in.
     returns SUCCESS_LOGIN if login is successful*/
int verify_login(unsigned char * username, unsigned char * password){
    if (user_list == NULL){
        fprintf(stderr, "Error: user_list is not initialized.\n");
        return LIST_EMPTY;
    }
    user_t* user = database_search_user(user_list, username);
    if (user == NULL){
        return USERNAME_ERROR;
    }
    if (strcmp((char*)user->password, (char*)password) == 0){
        if (user->status == ONLINE)
            return ALREADY_LOGIN;
        else
            return PASSWORD_ERROR;
        return SUCCESS_LOGIN;
    }

    return SUCCESS_LOGIN;
}

/* exit a user by clearing their statuses
 * this is part of logging out user. the
 * reset is done in the handler function*/
void server_side_user_exit(char * username){
    user_t* user = database_search_user(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    fprintf(stdout,"============================\n");
    printf("user %s is exiting.\n", user->username);
    if (user->in_session == ONLINE){
        server_side_session_leave(username);
    }
    pthread_mutex_lock(&user_list_mutex);
    user->status = OFFLINE;
    user->socket_fd = OFFLINE;
    user->port = OFFLINE;
    memset((char*)user->ip_address, 0, sizeof(user->ip_address));
    memset((char*)user->ip_address, 0, sizeof(user->ip_address));
    fprintf(stdout,"============================\n");
    pthread_mutex_unlock(&user_list_mutex);
}

void server_side_session_create(char * username, char * session_id){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = database_search_user(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    message_t reply_message;
    char buffer[maximum_buffer_size];
    int count = get_user_count_in_a_session(user_list, session_id, &rw_mutex);

    if(user->in_session == ONLINE){
        fill_message(&reply_message, JN_NAK,
                     strlen(session_response_message(USER_NAME_IN_SESSION)),
                     username,session_response_message(USER_NAME_IN_SESSION));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    else if (count == 0){
        memset((char*)user->session_id, 0, sizeof(user->session_id));
        strcpy((char*)user->session_id, session_id);
        user->in_session = ONLINE;
        user->role = ADMIN;
        fill_message(&reply_message, NS_ACK,
                     strlen(session_id),
                     username,session_id);
    }
    else if (count == SESSION_NAME_RESERVED){
        fill_message(&reply_message, NEW_SESS,
                     strlen(session_response_message(SESSION_NAME_RESERVED)),
                     username,session_response_message(SESSION_NAME_RESERVED));
    }
    else{
        fill_message(&reply_message, NEW_SESS,
                     strlen(session_response_message(SESSION_EXISTS)),
                     username,session_response_message(SESSION_EXISTS));
    }

    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&user->socket_fd, strlen(buffer), buffer);
    pthread_mutex_unlock(&user_list_mutex);
}

/* joins a user to a specific session
 * if a user is in a session, the
 * session doesn't exist or the session
 * is at its full cpacity, reply JN_NAK
 * returns a JN_ACK if passing all cases*/
void server_side_session_join(char * username, char * session_id){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = database_search_user(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    message_t reply_message;
    char buffer[maximum_buffer_size];

    int session_user_count = get_user_count_in_a_session(user_list, session_id, &rw_mutex);

    if (user->in_session == ONLINE){
        printf("user %s is already in a session.\n", user->username);
        fill_message(&reply_message, JN_NAK,
                     strlen(session_response_message(USER_NAME_IN_SESSION)),
                     username,session_response_message(USER_NAME_IN_SESSION));
    }
    else if (session_user_count == 0){
        printf("session %s does not exist.\n", session_id);
        fill_message(&reply_message, JN_NAK,
                     strlen(session_response_message(SESSION_NAME_INVALID)),
                     username,session_response_message(SESSION_NAME_INVALID));
    }
    else if (session_user_count == MAX_SESSION_USER){
        printf("session %s is full!\n", session_id);
        fill_message(&reply_message, JN_NAK,
                     strlen(session_response_message(SESSION_AT_FULL_CAPACITY)),
                     username,session_response_message(SESSION_AT_FULL_CAPACITY));
    }
    else{
        memset((char*)user->session_id, 0, sizeof(user->session_id));
        strcpy((char*)user->session_id, session_id);
        user->in_session = ONLINE;
        printf("user %s joined session %s.\n", user->username, user->session_id);
        fill_message(&reply_message, JN_ACK,
                     strlen(session_id),
                     username,session_id);
    }

    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&user->socket_fd, strlen(buffer), buffer);
    pthread_mutex_unlock(&user_list_mutex);
}

/* removes a user from a session
 * if a user is not in a session, the
 * session doesn't exist, we just return
 * otherwise remove the user*/
void server_side_session_leave(char *username){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = database_search_user(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    if (user->in_session == OFFLINE){
        return;
    }
    user_exit_current_session(user_list, username, &rw_mutex);
    pthread_mutex_unlock(&user_list_mutex);
}


/* returns a string based on the index
 * of the login error code*/
char* select_login_message(int index){
    if(index == USERNAME_ERROR)
        return "Your username does not appear to be valid.";
    else if(index == PASSWORD_ERROR)
        return "Your password is incorrect.";
    else if (index == ALREADY_LOGIN)
        return "You have logged in elsewhere.\n Please log out of the other session before proceeding.";
    else if (index == MULTIPLE_LOG_AT_SAME_IP)
        return "Logging into another account while staying online is not permitted!";
    else
        return NULL;
}

/* logs in a user by setting their status to online
 * and their socket file descriptor to the current
 * socket file descriptor*/
void user_login(char * username, unsigned char status, const int* socket_fd){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = database_search_user(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    user->socket_fd = *socket_fd;
    user->status = status;
    char* IP_ADDR_BUFFER;
    IP_ADDR_BUFFER = get_user_ip_address_and_port(&user->socket_fd, &user->port);
    strcpy(user->ip_address, IP_ADDR_BUFFER);
    free(IP_ADDR_BUFFER);
    pthread_mutex_unlock(&user_list_mutex);
    print_all_users_in_a_database(user_list);
}

/* returns a string based on the index
 * of the session error code*/
char* session_response_message(int value){
    switch (value) {
        case SESSION_NAME_RESERVED:
            return "Session name is reserved!.";
        case SESSION_NAME_INVALID:
            return "Session name is not found!.";
        case SESSION_AT_FULL_CAPACITY:
            return "Session you tried to join was full!.";
        case SESSION_EXISTS:
            return "Session already exists!.";
        case SESSION_ADD_USER_FAILURE:
            return "failed to add the user in the session!.";
        case USER_NAME_IN_SESSION:
            return "Please leave the session before joining/creating another one!.";
        case USER_DOES_NOT_EXIST:
            return "User does not exist!.";
        case USER_NOT_IN_SESSION:
            return "User is not in a session!.";
        case USER_NOT_ONLINE:
            return "User is not online!.";
        case USER_IN_DIFFERENT_SESSION:
            return "User is in a different session!.";
        case TARGET_IS_ADMIN:
            return "Target User is an admin.";
        default:
            return NULL;
    }
}

/* sends a message to all users in a session
 * by iterating through the user list and
 * checking if the user is in the session
 * and if the user is online*/
void send_message_in_a_session(message_t * msg, char * session_id ){
    pthread_mutex_lock(&user_list_mutex);
    char buffer [ACC_BUFFER_SIZE];
    message_to_string(msg, msg->size, buffer);
    for (int i = 0; i < user_list->count; ++i) {
        if (user_list->user_list[i].status == OFFLINE)
            continue;
        if(user_list->user_list[i].in_session == OFFLINE)
            continue;
        if (strcmp((char*)user_list->user_list[i].session_id, session_id) == 0){
            send_message(&(user_list->user_list[i].socket_fd), strlen(buffer), buffer);
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
}

/* sets up the database by reading and normalizing
 * the read line buffer and setting user info based off
 * normalized strings*/
int set_up_database(){
    fprintf(stdout, "Setting up database...\n");
    FILE *file_pointer;
    file_pointer = fopen(database_path, "r");
    if (file_pointer == NULL){
        free(user_list);
        perror("Error opening file");
        fclose(file_pointer);
        return FILE_ERROR;
    }
    char username[MAX_NAME];
    char password[MAX_PASSWORD];
    while(fscanf(file_pointer, "\"%[^|\"]\":\"%[^|\"]\"\n", username, password) != EOF){
        user_t new_user = {0};
        strcpy((char*)new_user.username, username);
        strcpy((char*)new_user.password, password);
        new_user.status = OFFLINE;
        new_user.in_session = OFFLINE;
        new_user.role = USER;
        new_user.socket_fd = OFFLINE;
        new_user.port = OFFLINE;
        strcpy((char*)new_user.session_id, NOT_IN_SESSION);
        memset((char*)new_user.ip_address, 0, sizeof(new_user.ip_address));
        user_database_add_user(user_list, &new_user, &rw_mutex);
    }
    return 1;
}

/* returns the ip address of a user
 * by using getpeername, setting the
 * port along the way*/
char* get_user_ip_address_and_port(const int * socket_fd,  unsigned int * port){
    int socket = *socket_fd;
    struct sockaddr_storage user_addr;
    memset(&user_addr, 0, sizeof(user_addr));
    char* ip_address_buffer = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
    socklen_t user_addrLen = sizeof(user_addr);

    if (getpeername(socket, (struct sockaddr *)&user_addr, &user_addrLen) == 0) {
        if (user_addr.ss_family == AF_INET) {
            // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) &user_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip_address_buffer, INET6_ADDRSTRLEN);
            *port = ntohs(ipv4->sin_port);
        } else if (user_addr.ss_family == AF_INET6) {
            // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) &user_addr;
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_address_buffer, INET6_ADDRSTRLEN);
            *port = ntohs(ipv6->sin6_port);
        } else {
            printf("Unknown address family\n");
        }
    }
    else{
        perror("Getpeername failed");
    }

    return ip_address_buffer;
}

/*   register a new user
     returns error code if the database is full
     or the username alreay exists*/
int user_registration(char * username, char * password, const int  *socket_fd){
    pthread_mutex_lock(&user_list_mutex);
    user_t new_user = {0};
    strcpy((char*)new_user.username, username);
    strcpy((char*)new_user.password, password);
    strcpy((char*)new_user.session_id, NOT_IN_SESSION);
    memset((char*)new_user.ip_address, 0, sizeof(new_user.ip_address));
    new_user.status = ONLINE;
    new_user.in_session = OFFLINE;
    new_user.role = USER;
    new_user.socket_fd = *socket_fd;
    new_user.port = OFFLINE;
    char* ip_address;
    ip_address = get_user_ip_address_and_port(&new_user.socket_fd, &new_user.port);
    strcpy(new_user.ip_address, ip_address);
    free(ip_address);
    int code = user_database_add_user(user_list, &new_user, &rw_mutex);
    if (code == USER_ALREADY_EXIST || code == USER_LIST_FULL){
        pthread_mutex_unlock(&user_list_mutex);
        return code;
    }
    print_all_users_in_a_database(user_list);
    wrtie_to_database(get_user_count_in_database(user_list,&rw_mutex) - 1);
    pthread_mutex_unlock(&user_list_mutex);
    return code;
}

/* writes a new user to the database file*/
void wrtie_to_database(int user_index){
    FILE * fp;
    fp = fopen(database_path, "a");
    if (fp == NULL){
        perror("Error opening file");
        return;
    }
    fprintf(fp, "\n\"%s\":\"%s\"", user_list->user_list[user_index].username, user_list->user_list[user_index].password);
    fclose(fp);
}

/* returns a string based on the index
 * of the registration error code*/
char* user_registration_message(int value){
    switch (value) {
        case USER_ALREADY_EXIST:
            return "Username already exists!.";
        case USER_LIST_FULL:
            return "User list is full!.";
        default:
            return NULL;
    }
}

/* kick a user from a session, sends a message
 * to the user based on the response code from
 * the database lookup*/
void server_side_kick_user(char* sender, char* target){
    pthread_mutex_lock(&user_list_mutex);
    user_t* target_user = database_search_user(user_list, (unsigned char*)target);
    user_t* sender_user = database_search_user(user_list, (unsigned char*)sender);

    message_t reply_message = {0};
    char buffer[maximum_buffer_size] = {0};

    if (target_user == NULL){
        fill_message(&reply_message, KI_NAK,
                     strlen(session_response_message(USER_DOES_NOT_EXIST)),
                     sender,session_response_message(USER_DOES_NOT_EXIST));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    if (target_user->status == OFFLINE){
        fill_message(&reply_message, KI_NAK,
                     strlen(session_response_message(USER_NOT_ONLINE)),
                     sender,session_response_message(USER_NOT_ONLINE));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    if (target_user->in_session == OFFLINE){
        fill_message(&reply_message, KI_NAK,
                     strlen(session_response_message(USER_NOT_IN_SESSION)),
                     sender,session_response_message(USER_NOT_IN_SESSION));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    else if (strcmp(target_user->session_id,sender_user->session_id) != 0){
        fill_message(&reply_message, KI_NAK,
                     strlen(session_response_message(USER_IN_DIFFERENT_SESSION)),
                     sender,session_response_message(USER_IN_DIFFERENT_SESSION));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    else if (target_user->role == ADMIN){
        fill_message(&reply_message, KI_NAK,
                     strlen(session_response_message(TARGET_IS_ADMIN)),
                     sender,session_response_message(TARGET_IS_ADMIN));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }

    target_user->in_session = OFFLINE;
    memset((char*)target_user->session_id, 0, sizeof(target_user->session_id));
    strcpy((char*)target_user->session_id, NOT_IN_SESSION);
    fill_message(&reply_message, KI_MESSAGE,
                 strlen(sender_user->session_id),
                 sender,sender_user->session_id);
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&target_user->socket_fd, strlen(buffer), buffer);

    memset(&reply_message, 0, sizeof(reply_message));
    memset(buffer, 0, sizeof(buffer));

    fill_message(&reply_message, KI_ACK,
                 strlen((char *)target_user->username),
                 sender,(char *)target_user->username);
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&sender_user->socket_fd, strlen(buffer), buffer);

    pthread_mutex_unlock(&user_list_mutex);
}

/* promote a user to be the owner of a session, sends a message
 * to the user based on the response code from
 * the database lookup*/
void server_side_promote_user(char* sender, char* target){
    pthread_mutex_lock(&user_list_mutex);
    user_t* target_user = database_search_user(user_list, (unsigned char*)target);
    user_t* sender_user = database_search_user(user_list, (unsigned char*)sender);

    message_t reply_message = {0};
    char buffer[maximum_buffer_size] = {0};

    if (target_user == NULL){
        fill_message(&reply_message, PM_NAK,
                     strlen(session_response_message(USER_DOES_NOT_EXIST)),
                     sender,session_response_message(USER_DOES_NOT_EXIST));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    if (target_user->status == OFFLINE){
        fill_message(&reply_message, PM_NAK,
                     strlen(session_response_message(USER_NOT_ONLINE)),
                     sender,session_response_message(USER_NOT_ONLINE));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    if (target_user->in_session == OFFLINE){
        fill_message(&reply_message, PM_NAK,
                     strlen(session_response_message(USER_NOT_IN_SESSION)),
                     sender,session_response_message(USER_NOT_IN_SESSION));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    else if (strcmp(target_user->session_id,sender_user->session_id) != 0){
        fill_message(&reply_message, PM_NAK,
                     strlen(session_response_message(USER_IN_DIFFERENT_SESSION)),
                     sender,session_response_message(USER_IN_DIFFERENT_SESSION));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    else if (target_user->role == ADMIN){
        fill_message(&reply_message, PM_NAK,
                     strlen(session_response_message(TARGET_IS_ADMIN)),
                     sender,session_response_message(TARGET_IS_ADMIN));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&sender_user->socket_fd, strlen(buffer), buffer);
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }

    target_user->role = ADMIN;
    sender_user->role = USER;
    fill_message(&reply_message, PM_MESSAGE,
                 0,sender,NULL);
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&target_user->socket_fd, strlen(buffer), buffer);

    memset(&reply_message, 0, sizeof(reply_message));
    memset(buffer, 0, sizeof(buffer));

    fill_message(&reply_message, PM_ACK,
                 strlen((char *)target_user->username),
                 sender,(char *)target_user->username);
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&sender_user->socket_fd, strlen(buffer), buffer);

    pthread_mutex_unlock(&user_list_mutex);
}
