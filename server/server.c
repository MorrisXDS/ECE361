#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"
#include "session.h"

user_list_t* user_list;
session_list_t* session_list;

int main(int argc, char* argv[]){
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
            perror("setsockopt"); // If setting the option fails, print an error recevied_message.
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

    session_list = session_list_init();
    if (session_list == NULL){
        perror("session_list_init");
        exit(errno);
    }
    fprintf(stdout, "session list initialized\n");

    printf("database initialized!\n");
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
    user_list_remove_all(user_list);
    session_list_remove_all(session_list);
    free(session_list);
    for (int i = 0; i < thread_count; i++){
        pthread_join(thread_pool[i], NULL);
    }
    pthread_exit(NULL);
    return 1;
}

void* connection_handler(void* accept_fd){
    int socket_file_descriptor = *(int *)accept_fd;
    message_t recevied_message, reply_message;
    char buffer[maximum_buffer_size];
    char source[MAX_NAME];
    char connection_status = OFFLINE;
    unsigned message_type = 999;
    user_t* user;

    while(1){
        memset(buffer, 0, sizeof(buffer));
        memset(&recevied_message, 0, sizeof(recevied_message));
        memset(&reply_message, 0, sizeof(reply_message));
        size_t bytes_received = receive_message(&socket_file_descriptor, buffer);
        //size_t bytes_received = recv(socket_file_descriptor, buffer, sizeof(buffer), 0);
        if (bytes_received == CONNECTION_REFUSED){
            if (connection_status == ONLINE){
                server_side_user_exit(source);
            }
            break;
        }
        if (bytes_received == RECEIVE_ERROR){
            perror("recv");
            break;
        }

        printf("buffer: %s\n", buffer);

        string_to_message(&recevied_message, buffer);
        if (recevied_message.type == LOGIN){
            if (connection_status == ONLINE){
                message_type = LO_NAK;
                fill_message(&reply_message, message_type,
                             strlen(select_login_message(ALREADY_LOGIN)),
                             source, select_login_message(ALREADY_LOGIN));
                message_to_string(&recevied_message, recevied_message.size, buffer);
                send_message(&socket_file_descriptor,sizeof(buffer), buffer);
                continue;
            }
            int reply_type = verify_login(recevied_message.source,recevied_message.data);

            if (reply_type == SUCCESS_LOGIN)
            {
                fprintf(stdout,"============================\n");
                fprintf(stdout, "Verification Success!\n");
                connection_status = ONLINE;
                message_type = LO_ACK;
                strcpy(source, (char*)recevied_message.source);
                fprintf(stdout, "name identified!!\n");
                user_log_in(source, (char*)recevied_message.data, ONLINE, &socket_file_descriptor);
                fprintf(stdout, "user %s just landed!!\n", source);
                fprintf(stdout,"============================\n");
                fill_message(&reply_message,message_type,0, (char *)recevied_message.source, NULL);
            }
            else{
                message_type = LO_NAK;
                fill_message(&reply_message, message_type,
                             strlen(select_login_message(reply_type)),
                             (char *)recevied_message.source, select_login_message(reply_type));
            }
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        if (recevied_message.type == EXIT){
            server_side_user_exit(source);
            break;
        }
        if (recevied_message.type == JOIN){
            server_side_session_join(source, (char*)recevied_message.data);
            continue;
        }
        if (recevied_message.type == NEW_SESS){
            server_side_session_create(source, (char*)recevied_message.data);
            continue;
        }
        if (recevied_message.type == LEAVE_SESS){
            server_side_session_leave(source);
            continue;
        }
        if (recevied_message.type == QUERY){
            char list[maximum_buffer_size];
            get_active_list(user_list, list);
            fill_message(&reply_message, QU_ACK,
                         strlen(list), (char *)recevied_message.source, list);
            message_to_string(&reply_message, reply_message.size, buffer);
        }
        if (recevied_message.type == MESSAGE){
            fill_message(&reply_message, MESSAGE,
                         strlen((char*)recevied_message.data),
                         (char *)recevied_message.source, (char*)recevied_message.data);
            user_t* sedning_user = return_user_by_username(user_list, recevied_message.source);
            send_message_in_a_session(&reply_message, sedning_user->session_id);
            continue;
        }



        size_t bytes_sent = send_message(&socket_file_descriptor,strlen(buffer),buffer);
        fprintf(stdout, "bytes sent: %zu\n", bytes_sent);
        if (bytes_received == CONNECTION_REFUSED){
            if (connection_status == ONLINE){
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
    return NULL;
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
        user_t temp_user = {0};
        create_user(&temp_user,(unsigned char*)username,
                    (unsigned char*)password, OFFLINE,
                    NOT_IN_SESSION, NULL, OFFLINE,OFFLINE, USER);
        user_t temp = {0};

        create_user(&temp,(unsigned char*)username,
                    (unsigned char*)password, OFFLINE,
                    NOT_IN_SESSION, NULL, OFFLINE,OFFLINE, USER);
        user_list_add_user(user_list, temp_user);
    }
    return 1;
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
    if (username == NULL){
        fprintf(stderr, "Error: username is NULL.\n");
        return;
    }
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    fprintf(stdout,"============================\n");
    printf("user %s is exiting.\n", user->username);
    user->status = OFFLINE;
    user->socket_fd = OFFLINE;
    user->port = OFFLINE;
    user->role = USER;
    session_list_remove_user(session_list,(char*)user->session_id, (char*)user->username);
    session_status_t* session = session_list_find(session_list, (char*)user->session_id);
    if (session != NULL){
        if (session->user_list->head == NULL){
            session_list_remove(session_list, session);
        }
    }
    strcpy((char*)user->session_id, NOT_IN_SESSION);
    memset((char*)user->ip_address, 0, sizeof(user->ip_address));
    strcmp((char*)user->session_id, NOT_IN_SESSION);
    fprintf(stdout,"============================\n");
}




void server_side_session_create(char * username, char * session_id){
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    session_status_t status;
    create_session_status(&status, session_id, user);
    printf("user %s is creating session %s.\n", user->username, session_id);
    int code = session_list_insert_at_tail(session_list, &status);
    printf("code: %d\n", code);
    message_t reply_message;
    char buffer[maximum_buffer_size];
    if (code == 0){
        char name[maximum_buffer_size];
        sprintf(name, "================================\n"
                      "You have just created session %s\n. You become the admin", session_id);
        fill_message(&reply_message, NS_ACK, strlen(name), username, name);
        strcpy((char*)user->session_id, session_id);
        fprintf(stdout, "user session has been updated to %s.\n",session_id);
        user->role = ADMIN;
    }
    else{
        fill_message(&reply_message, NEW_SESS,
                     strlen(session_response_message(code)), username,
                     session_response_message(code));
    }
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&user->socket_fd, sizeof (buffer), buffer);

}

void server_side_session_join(char * username, char * session_id){
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    session_status_t* status;
    message_t reply_message;
    char buffer[maximum_buffer_size];

    printf("session_id: %s\n", session_id);
    status = session_list_find(session_list, session_id);

    if (status == NULL){
        fprintf(stderr, "Error: session is empty.\n");
        return;
    }
    if (strcmp((char*)user->session_id, NOT_IN_SESSION) != 0){
        fprintf(stderr, "Error: user is already in a session.\n");
        fill_message(&reply_message, JN_NAK,
                     strlen(session_response_message(SESSION_ADD_USER_FAILURE)),
                     username,session_response_message(SESSION_ADD_USER_FAILURE));
        message_to_string(&reply_message, reply_message.size, buffer);
        send_message(&user->socket_fd, sizeof(buffer), buffer);
        return;
    }
    int code = session_list_session_add_user(session_list, session_id, user);

    if (code == SESSION_ADD_USER_SUCCESS){
        fill_message(&reply_message, JN_ACK,
                     sizeof(session_id), username, session_id);
        strcpy((char*)user->session_id, session_id);
    }
    else{
        fill_message(&reply_message, JN_NAK,
                     strlen(session_response_message(code)), (char*)user->username,
                     session_response_message(code));
    }
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(&user->socket_fd, sizeof(buffer), buffer);

}

void server_side_session_leave(char *username){
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    if (strcmp((char*)user->session_id, NOT_IN_SESSION) != 0){
        session_list_remove_user(session_list, user->session_id, (char *)user->username);
        user_t* log_in_user = user_list_find(user_list, (unsigned char*)username);
        log_in_user->role = USER;
        strcpy((char*)log_in_user->session_id, NOT_IN_SESSION);
        return;
    }
}


//int session_name_check(char * session_id);
void print_active_list(user_list_t * list){
    if (list == NULL){
        fprintf(stderr, "Error: list is NULL.\n");
        return;
    }
    fprintf(stdout, "============================\n");
    fprintf(stdout, "Ative User List:\n");
    fprintf(stdout, "%*s %*s\n", -MAX_NAME, "Username", -MAX_SESSION_NAME, "Session ID");

    link_user_t * current = list->head;
    while(current != NULL){
        if (current->user->status == OFFLINE){
            current = current->next;
            continue;
        }
        printf("%*s", -MAX_NAME, current->user->username);
        printf("%*s\n", -MAX_SESSION_NAME, current->user->session_id);
        current = current->next;
    }
}

void get_active_list(user_list_t * list, char * buffer){
    if (list == NULL){
        fprintf(stderr, "Error: list is NULL.\n");
        return;
    }
    strcat(buffer, "Active User List:\n");
    char message[maximum_buffer_size];
    sprintf(message, "%*s %*s\n", -MAX_NAME, "Username", -MAX_SESSION_NAME, "Session ID");
    strcat(buffer, message);
    link_user_t * current = list->head;
    while(current != NULL){
        if (current->user->status == OFFLINE){
            current = current->next;
            continue;
        }
        memset(message, 0, sizeof(message));
        sprintf(message, "%*s %*s\n", -MAX_NAME,
                current->user->username,
                -MAX_SESSION_NAME, current->user->session_id);
        strcat(buffer, message);
        current = current->next;
    }
    fprintf(stdout, "%s", buffer);
}

char* select_login_message(int index){
    if(index == USERNAME_ERROR)
        return "Your username does not appear to be valie.";
    else if(index == PASSWORD_ERROR)
        return "Your password is incorrect.";
    else if (index == ALREADY_LOGIN)
        return "You have logged in elsewhere.\n Please log out of the other session before proceeding.";
    else
        return NULL;
}

void user_log_in(char * username, char * password, unsigned char status, int* socket_fd){
    user_t* user = user_list_find(user_list, (unsigned char*)username);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    user->socket_fd = *socket_fd;
    user->status = ONLINE;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(*socket_fd, (struct sockaddr*)&client_addr, &addr_len) == -1) {
        perror("getpeername");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
    user->port = ntohs(s->sin_port);
    inet_ntop(AF_UNSPEC, &s->sin_addr, user->ip_address,
              sizeof (user->ip_address));
    print_active_list(user_list);
}

char* session_response_message(int value){
    if (value == SESSION_NAME_INVALID)
        return "Session name is invalid!.";
    else if (value == SESSION_NAME_RESERVED)
        return "Session name is reserved!.";
    else if (value == SESSION_NAME_TOO_LONG)
        return "Session name is too long!.";
    else if (value == SESSION_LIST_FULL)
        return "Session list is full!.";
    else if (value == SESSION_EXISTS)
        return "Session already exists!.";
    else if (value == SESSION_CREATE_FAIL)
        return "Session creation failed!.";
    else if (value == SESSION_ADD_USER_SUCCESS)
        return "User added to session successfully!.";
    else if (value == SESSION_ADD_USER_FAILURE)
        return "failed to add the user in the section!.";
    else
        return NULL;
}

void send_message_in_a_session(message_t * msg, char * session_id){
    char buffer [ACC_BUFFER_SIZE];
    message_to_string(msg, msg->size, buffer);
    session_status_t * session = session_list_find(session_list, session_id);
    if (session == NULL){
        fprintf(stderr, "Error: session is NULL.\n");
        return;
    }
    link_user_t * current_user = session->user_list->head;
    if (current_user == NULL){
        fprintf(stderr, "Error: No online user to talk to !.\n");
        return;
    }
    while (current_user != NULL){
        send_message(&current_user->user->socket_fd, sizeof(buffer), buffer);
        current_user = current_user->next;
    }
}