#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"
#include "user.h"

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

    user_list = user_list_init();
    if (user_list == NULL){
        perror("user_list_init");
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
        if (thread_count == THREAD_CAPACITY) {
            printf("thread pool full\n");
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
    user_list_remove_all(user_list, &user_list_mutex);
    for (int i = 0; i < thread_count; i++){
        pthread_join(thread_pool[i], NULL);
    }
    pthread_exit(NULL);
    return 1;
}

void* connection_handler(void* accept_fd){
    int socket_file_descriptor = *(int *)accept_fd;
    message_t received_message, reply_message;
    char buffer[maximum_buffer_size];
    char source[MAX_NAME];
    char server_connection_status = OFFLINE;
    unsigned message_type = 999;
    user_t* user;

    while(1){
        memset(buffer, 0, sizeof(buffer));
        memset(&received_message, 0, sizeof(received_message));
        memset(&reply_message, 0, sizeof(reply_message));
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
            perror("recv");
            break;
        }

        string_to_message(&received_message, buffer);

        if (received_message.type == LOGIN){
            int reply_type = verify_login(received_message.source,received_message.data);
            if (server_connection_status == ONLINE){
                message_type = LO_NAK;
                fill_message(&reply_message, message_type,
                             strlen(select_login_message(ALREADY_LOGIN)),
                             source, select_login_message(ALREADY_LOGIN));
                message_to_string(&received_message, received_message.size, buffer);
                send_message(&socket_file_descriptor,strlen(buffer), buffer);
                continue;
            }



            if (reply_type == SUCCESS_LOGIN)
            {
                fprintf(stdout,"============================\n");
                fprintf(stdout, "Verification Success!\n");
                server_connection_status = ONLINE;
                message_type = LO_ACK;
                user_login((char *)received_message.source, (char*)received_message.data, ONLINE, &socket_file_descriptor);
                strcpy(source, (char*)received_message.source);
                fprintf(stdout, "user %s just landed!!\n", source);
                fprintf(stdout,"============================\n");
                fill_message(&reply_message,message_type,0, (char *)received_message.source, NULL);
            }
            else{
                message_type = LO_NAK;
                fill_message(&reply_message, message_type,
                             strlen(select_login_message(reply_type)),
                             (char *)received_message.source, select_login_message(reply_type));
            }
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        if (received_message.type == EXIT){
            server_side_user_exit(source);
            break;
        }
        if (received_message.type == JOIN){
            server_side_session_join(source, (char*)received_message.data);
            continue;
        }
        if (received_message.type == NEW_SESS){
            server_side_session_create(source, (char*)received_message.data);
            continue;
        }
        if (received_message.type == LEAVE_SESS){
            server_side_session_leave(source);
            continue;
        }
        if (received_message.type == QUERY){
            char list[maximum_buffer_size];
            get_active_user_list(user_list, list);
            fill_message(&reply_message, QU_ACK,
                         strlen(list), (char *)received_message.source, list);
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        if (received_message.type == MESSAGE){
            fill_message(&reply_message, MESSAGE,
                         received_message.size,
                         (char *)received_message.source, (char*)received_message.data);
            user_t* sedning_user = return_user_by_username(user_list, received_message.source);
            send_message_in_a_session(&reply_message, sedning_user->session_id);
            continue;
        }



        size_t bytes_sent = send_message(&socket_file_descriptor,strlen(buffer),buffer);
        if (bytes_received == CONNECTION_REFUSED){
            if (server_connection_status == ONLINE){
                server_side_user_exit(source);
            }
            break;
            break;
        }
        if (bytes_received == SEND_ERROR){
            perror("recv");
            break;
        }

    }
    close(socket_file_descriptor);
    fprintf(stdout, "thread exited\n");
    return NULL;
}

int verify_login(unsigned char * username, unsigned char * password){
    if (user_list == NULL){
        fprintf(stderr, "Error: user_list is not initialized.\n");
        return LIST_EMPTY;
    }
    user_t* user = user_list_find(user_list, username);
    if (user == NULL){
        return USERNAME_ERROR;
    }
    if (strcmp((char*)user->password, (char*)password) == 0){
        if (user->status == ONLINE)
            return ALREADY_LOGIN;
        return SUCCESS_LOGIN;
    }

    return SUCCESS_LOGIN;
}

void server_side_user_exit(char * username){
    user_t* user = user_list_find(user_list, (unsigned char*)username);
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
    user_list_print(user_list);
}




void server_side_session_create(char * username, char * session_id){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    message_t reply_message;
    char buffer[maximum_buffer_size];
    int count = user_list_count_by_session_id(user_list, session_id, &rw_mutex);

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

void server_side_session_join(char * username, char * session_id){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        pthread_mutex_unlock(&user_list_mutex);
        return;
    }
    message_t reply_message;
    char buffer[maximum_buffer_size];

    int session_user_count = user_list_count_by_session_id(user_list, session_id, &rw_mutex);

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

void server_side_session_leave(char *username){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = user_list_find(user_list, (unsigned char*)username);
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


char* select_login_message(int index){
    if(index == USERNAME_ERROR)
        return "Your username does not appear to be valie.";
    else if(index == PASSWORD_ERROR)
        return "Your password is incorrect.";
    else if (index == ALREADY_LOGIN)
        return "You have logged in elsewhere.\n Please log out of the other session before proceeding.";
    else if (index == MULTIPLE_LOG_AT_SAME_IP)
        return "Logging into another account while staying online is not permitted!";
    else
        return NULL;
}

void user_login(char * username, char * password, unsigned char status, int* socket_fd){
    pthread_mutex_lock(&user_list_mutex);
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    user->socket_fd = *socket_fd;
    user->status = ONLINE;
    char* IP_ADDR_BUFFER;
    IP_ADDR_BUFFER = get_user_ip_address_and_port(&user->socket_fd, &user->port);
    strcpy(user->ip_address, IP_ADDR_BUFFER);
    free(IP_ADDR_BUFFER);
    pthread_mutex_unlock(&user_list_mutex);
}

char* session_response_message(int value){
    if (value == SESSION_NAME_RESERVED)
        return "Session name is reserved!.";
    if (value == SESSION_NAME_INVALID)
        return "Session name is not found!.";
    if (value == SESSION_AT_FULL_CAPACITY)
        return "Session you tried to join was full!.";
    if (value == SESSION_EXISTS)
        return "Session already exists!.";
    if (value == SESSION_ADD_USER_FAILURE)
        return "failed to add the user in the session!.";
    if (value == USER_NAME_IN_SESSION)
        return "Please leave the session before joining/createing another one!.";
    else
        return NULL;
}

void send_message_in_a_session(message_t * msg, char * session_id ){
    pthread_mutex_lock(&user_list_mutex);
    char buffer [ACC_BUFFER_SIZE];
    int size = message_to_string(msg, msg->size, buffer);
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
    char buffer[maximum_buffer_size];
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
        user_t temp = {0};
        user_list_add_user(user_list, &new_user, &rw_mutex);
    }
    return 1;
}

char* get_user_ip_address_and_port(int * socket_fd,  unsigned int * port){
    int socket = *socket_fd;
    struct sockaddr_storage user_addr;
    char* ip_address_buffer = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
    socklen_t user_addrLen;

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

void user_registration(char * username, char * password, int socket_fd){
    pthread_mutex_lock(&user_list_mutex);
    user_t new_user = {0};
    strcpy((char*)new_user.username, username);
    strcpy((char*)new_user.password, password);
    new_user.status = ONLINE;
    new_user.in_session = OFFLINE;
    new_user.role = USER;
    new_user.socket_fd = socket_fd;
    new_user.port = OFFLINE;
    strcpy((char*)new_user.session_id, NOT_IN_SESSION);
    memset((char*)new_user.ip_address, 0, sizeof(new_user.ip_address));
    user_t temp = {0};
    user_list_add_user(user_list, &new_user, &rw_mutex);
    write_to_file(user_list_count(user_list,&rw_mutex) - 1);
    pthread_mutex_unlock(&user_list_mutex);
}

void write_to_file(int user_index){
    FILE * fp;
    fp = fopen(database_path, "a");
    if (fp == NULL){
        perror("Error opening file");
        return;
    }
    fprintf(fp, "\n\"%s\":\"%s\"", user_list->user_list[user_index].username, user_list->user_list[user_index].password);
    fclose(fp);
}