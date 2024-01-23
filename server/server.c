#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"
#include "session.h"

user_t* user_list;

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

    if (!user_list_init()){
        fprintf(stderr, "Error initializing database. Server Crashed!\n");
        exit(-1);
    }
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
    free(user_list);
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
                server_side_user_exit(return_user_index(source));
            }
            break;
        }
        if (bytes_received == RECEIVE_ERROR){
            perror("recv");
            break;
        }

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
            server_side_user_exit(return_user_index(source));
            break;
        }


        size_t bytes_sent = send_message(&socket_file_descriptor,strlen(buffer),buffer);
        if (bytes_received == CONNECTION_REFUSED){
            if (connection_status == ONLINE){
                server_side_user_exit(return_user_index(source));
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

int user_list_init(){
    user_list = malloc(USER_LIST_CAPACITY * sizeof(user_t));
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
        unsigned int index = get_user_count();
        add_user(username, password, OFFLINE, OFFLINE, NULL, OFFLINE);
        if (USER_LIST_CAPACITY == index){
            if(!expand_user_list())
                return errno;
        }
        if (index == get_user_count()){
            fclose(file_pointer);
            return FILE_ERROR;
        }
    }
    return 1;
}

void add_user(char * username, char * password, unsigned char status, int socket_fd, char * ip_address, unsigned int port){
    if (user_list == NULL){
        fprintf(stderr, "Error: user_list is not initialized.\n");
        return;
    }
    int index = get_user_count();
    if (index == USER_LIST_CAPACITY){
        if(expand_user_list() != 1)
            return;
    }
    strcpy((char*)user_list[index].username, username);
    strcpy((char*)user_list[index].password, password);
    strcpy((char*)user_list[index].session_id, NOT_IN_SESSION);
    if (ip_address == NULL)
        strcpy((char*)user_list[index].ip_address, IP_NOT_FOUND);
    else
        strcpy((char*)user_list[index].ip_address, ip_address);
    user_list[index].port = port;
    user_list[index].status = status;
    user_list[index].socket_fd = socket_fd;
    user_list[index].role = USER;
    increment_user_count();
}

int expand_user_list(){
    USER_LIST_CAPACITY *= 2;
    user_list = realloc(user_list, USER_LIST_CAPACITY * sizeof(user_t));
    if (user_list == NULL){
        perror("Error: realloc failed.\n");
        return errno;
    }
    return 1;
}

int verify_login(unsigned char * username, unsigned char * password){
    if (user_list == NULL){
        fprintf(stderr, "Error: user_list is not initialized.\n");
        return LIST_EMPTY;
    }
    int count = get_user_count();
    for (int i = 0; i < count; i++){
        if (strcmp((char*)user_list[i].username, (char*)username) == 0){
            if (strcmp((char*)user_list[i].password, (char*)password) == 0){
                if (user_list[i].status == ONLINE)
                    return ALREADY_LOGIN;
                return SUCCESS_LOGIN;
            }
            else{
                return PASSWORD_ERROR;
            }
        }
    }
    return USERNAME_ERROR;
}


unsigned int get_user_count(){
    return USER_COUNT;
}
unsigned int increment_user_count(){
    USER_COUNT++;
    return USER_COUNT;
}
unsigned int decrement_user_count(){
    USER_COUNT--;
    return USER_COUNT;
}

void server_side_user_exit(int id){
    if (user_id_bound_check(id) == 0){
        return;
    }
    fprintf(stdout,"============================\n");
    printf("user %s is exiting.\n", user_list[id].username);
    user_list[id].status = OFFLINE;
    user_list[id].socket_fd = OFFLINE;
    user_list[id].port = OFFLINE;
    user_list[id].role = USER;
    //TODO: implement session stuff
    memset((char*)user_list[id].ip_address, 0, sizeof(user_list[id].ip_address));
    memset((char*)user_list[id].session_id, 0, sizeof(user_list[id].session_id));
    strcmp((char*)user_list[id].session_id, NOT_IN_SESSION);
    fprintf(stdout,"============================\n");
}

void server_side_session_leave(int id){
    if (user_id_bound_check(id) == 0){
        return;
    }
    user_t* user = return_user_by_id(id);
    if (user == NULL){
        fprintf(stderr, "Error: user is NULL.\n");
        return;
    }
    if (strcmp((char*)user->session_id, NOT_IN_SESSION) != 0){
        //TODO: implement session stuff
        return;
    }
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

user_t* return_user_by_id(int id){
    if (user_id_bound_check(id) == 0)
        return NULL;
    return &user_list[id];
}

int user_id_bound_check(int id){
    if (user_list == NULL){
        fprintf(stderr, "Error: user_list is not initialized.\n");
        return 0;
    }
    if(id < 0){
        fprintf(stderr, "Error: user id is out of bounds, < 0.\n");
        return 0;
    }
    if (id >= get_user_count()){
        fprintf(stderr, "Error: user id is out of bounds, user index non-existent.\n");
        return 0;
    }
    return 1;
}

int return_user_index(char * username){
    if (user_list == NULL){
        fprintf(stderr, "Error: user_list is not initialized.\n");
        return -1;
    }
    int count = get_user_count();
    for (int i = 0; i < count; i++){
        if (strcmp((char*)user_list[i].username, username) == 0){
            return i;
        }
    }
    return -1;
}

void user_log_in(char * username, char * password, unsigned char status, int* socket_fd){
    int index = return_user_index(username);
    user_list[index].socket_fd = *socket_fd;
    user_list[index].status = ONLINE;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(*socket_fd, (struct sockaddr*)&client_addr, &addr_len) == -1) {
        perror("getpeername");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
    user_list[index].port = ntohs(s->sin_port);
    inet_ntop(AF_UNSPEC, &s->sin_addr, user_list[index].ip_address,
              sizeof (user_list[index].ip_address));
    add_user(username, password, ONLINE, *socket_fd, user_list[index].ip_address, user_list[index].port);
}