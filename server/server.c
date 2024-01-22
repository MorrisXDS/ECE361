#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"
#include "session.h"


user_t user_list[LIST_CAPACITY];
session_list_t *list;
int user_count = 0;

pthread_mutex_t login_mutex;
pthread_mutex_t logout_mutex;
pthread_mutex_t join_mutex;
pthread_mutex_t leave_mutex;
pthread_mutex_t create_mutex;
pthread_mutex_t message_mutex;

int main(int argc, char* argv[]){
    //cited from Page 21 and Page 28, Beej's Guide to Network Programming, with modifications
    int status;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results
    list = session_list_init();

    int result = pthread_mutex_init(&logout_mutex, NULL);
    if (result != 0) {
        perror("failed to initialize logout mutex");
        exit(1);
    }

    result = pthread_mutex_init(&login_mutex, NULL);
    if (result != 0) {
        perror("failed to initialize login mutex");
        exit(1);
    }

    result = pthread_mutex_init(&join_mutex, NULL);
    if (result != 0) {
        perror("failed to initialize join mutex");
        exit(1);
    }

    result = pthread_mutex_init(&leave_mutex, NULL);
    if (result != 0) {
        perror("failed to initialize leave mutex");
        exit(1);
    }

    result = pthread_mutex_init(&create_mutex, NULL);
    if (result != 0) {
        perror("failed to initialize create mutex");
        exit(1);
    }

    result = pthread_mutex_init(&message_mutex, NULL);
    if (result != 0) {
        perror("failed to initialize message mutex");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    //end of citation

    int code = set_user_list();
    int check = error_check(code, SETUP_ERROR,
                            "failed to set up the user list");
    if (!check) exit(1);


    int listen_fd = socket(servinfo->ai_family,
                           servinfo->ai_socktype, servinfo->ai_protocol);
    check = error_check(listen_fd, FD_ERROR,
                        "failed to create a socket");
    if (!check) exit(1);


    // This code is cited from Page 26 of Beej's Guide to Network Programming.
    // It sets the option on the socket to allow the reuse of the local address in case
    // the server program is restarted before the old socket times out.
    int yes=1;
    // char yes='1'; // Solaris people use this

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt"); // If setting the option fails, print an error message.
        exit(1); // Exit the program with an error code.
    }
    // end of citation

    int condition = bind(listen_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    check = error_check(condition, BIND_ERROR,
                        "failed to bind the socket to the specified address");
    if (!check) exit(1);

    condition = listen(listen_fd, CONNECTION_CAPACITY);
    check = error_check(condition, LISTEN_ERROR,
                        "failed to listen to the port");
    if (!check) exit(1);

    addr_size = sizeof(their_addr);

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
        pthread_create(&thread_pool[thread_count], NULL, connection_handler, (void *)&accept_fd);
        thread_count++;
    }
    //wait for other threads to finish before exiting
    freeaddrinfo(servinfo);
    session_list_remove_all(list);
    pthread_exit(NULL);
    return 1;
}

int set_user_list(){
    FILE *fp = fopen("database.txt", "r");
    if (fp == NULL) return SETUP_ERROR;
    char username[MAX_NAME];
    char password[MAX_PASSWORD_LENGTH];
    while (fscanf(fp, "\"%[^\"]\":\"%[^\"]\"\n", username, password) != EOF){
        printf("user No.: %d\n", user_count+1);
        strcpy((char *)user_list[user_count].username, username);
        strcpy((char *)user_list[user_count].password, password);
        printf("username: %s\n", user_list[user_count].username);
        printf("password: %s\n", user_list[user_count].password);
        user_list[user_count].status = OFFLINE;
        user_list[user_count].socket_fd = OFFLINE;
        strcpy(user_list[user_count].session_id, NOT_IN_SESSION);
        user_count++;
    }
    fclose(fp);
    return 1;
}

int verify_login(unsigned char * username, unsigned char * password){
    for (int i = 0; i < user_count; ++i) {
        if (strcmp((char *)user_list[i].username, (char *)username) == 0){
            if (strcmp((char *)user_list[i].password, (char *)password) == 0){
                if (user_list[i].status == ONLINE){
                    return ALREADY_LOGIN;
                }
                user_list[i].status = ONLINE;
                return SUCCESS_LOGIN;
            } else {
                return PASSWORD_ERROR;
            }
        }
    }
    return USERNAME_ERROR;
}

void* connection_handler(void* accept_fd){
    int socekt_fd = *(int *)accept_fd;
    int authentication = 0;
    unsigned char data[ACC_BUFFER_SIZE];

    while(1){
        char name[MAX_NAME];
        ssize_t bytes_recv = -1;
        ssize_t bytes_sent = -1;
        memset(data, 0, ACC_BUFFER_SIZE);
        puts("waiting for a message");
        bytes_recv = recv(socekt_fd, data, ACC_BUFFER_SIZE, 0);
        printf("bytes received: %zd\n",bytes_recv);
        if (bytes_recv == 0) {
            puts("client disconnected");
            puts("closing the socket");
            puts("exiting the thread");
            break;
        }

        int check = error_check((int)bytes_recv, RECEIVE_ERROR,
                                "failed to receive the message");
        if (!check) break;

        message_t msg;
        printf("data: %s\n", data);
        buffer_to_message(&msg, data);
        memset(data, 0, ACC_BUFFER_SIZE);
        strcpy(name, (char *)msg.source);


        if(msg.type == LOGIN) {
            pthread_mutex_lock(&login_mutex);
            authentication = verify_login(msg.source, msg.data);
            printf("authentication: %d\n", authentication);
            if (authentication == ALREADY_LOGIN) {
                fill_message(&msg,LO_NAK, sizeof(login_error_types[ALREADY_LOGIN]),
                             (char *)msg.source, login_error_types[ALREADY_LOGIN]);
            }
            if (authentication == SUCCESS_LOGIN) {
                int index = return_user_index(name);
                user_list[index].socket_fd = socekt_fd;
                user_list[index].status = ONLINE;
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);

                if (getpeername(socekt_fd, (struct sockaddr*)&client_addr, &addr_len) == -1) {
                    perror("getpeername");
                    exit(EXIT_FAILURE);
                }
                struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
                user_list[index].port = ntohs(s->sin_port);
                inet_ntop(AF_INET, &s->sin_addr, user_list[index].ip_address,
                          sizeof (user_list[index].ip_address));
            }
            pthread_mutex_unlock(&login_mutex);
            generate_login_response(authentication,name, &msg, data, ACC_BUFFER_SIZE);
        }
        if(msg.type == EXIT){
            printf("%s closed connection\n", msg.source);
            puts("server ending communication");
            int index = return_user_index(name);
            user_list[index].socket_fd = OFFLINE;
            user_list[index].status = OFFLINE;
            if (strcmp(user_list[index].session_id,NOT_IN_SESSION) != 0){
                session_status_t * session_to_leave = session_list_find(list, user_list[index].session_id);
                pthread_mutex_lock(&leave_mutex);
                session_to_leave->user_count--;
                printf("there is %d people still in the session", session_to_leave->user_count);
                if (session_to_leave->user_count == 0){
                    printf("session #%s taken down\n", session_to_leave->session_id);
                    session_list_remove(list, session_to_leave);
                }
                pthread_mutex_unlock(&leave_mutex);
                strcpy(user_list[index].session_id, NOT_IN_SESSION);
                memset(user_list[index].ip_address, 0, INET_ADDRSTRLEN);
                memset(&user_list[index].port, 0, sizeof (user_list[index].port));
            }
            break;
        }
        if (msg.type == QUERY){
            get_active_user_list(&msg);
        }
        if (msg.type == NEW_SESS){
            pthread_mutex_lock(&create_mutex);
            int count = session_list_count(list);
            session_status_t* session = session_list_find(list, (char *)msg.data);
            if (count == MAX_SESSION_CAPACITY){
                char message [MAX_DATA] = "sessions at full capacity!";
                fill_message(&msg, JN_NAK, strlen(message),
                             (char *)msg.source, message);
            }
            else if (session != NULL){
                char message [MAX_DATA] = "session already exists!";
                fill_message(&msg, JN_NAK, strlen(message),
                             (char *)msg.source, message);
            }
            else{
                int index = return_user_index(name);
                pthread_mutex_lock(&join_mutex);
                session_status_t new_session;
                create_session_status(&new_session, (char *)msg.data);
                session_list_insert_at_head(list,&new_session);
                strcpy(user_list[index].session_id, (char *)msg.data);
                pthread_mutex_unlock(&join_mutex);
                msg.type = NS_ACK;
            }
            pthread_mutex_unlock(&create_mutex);
        }
        if (msg.type == JOIN){
            int count = session_list_count(list);
            session_status_t* session_to_join = session_list_find(list, (char *)msg.data);
            if (count == MAX_SESSION_CAPACITY){
                char message [MAX_DATA];
                sprintf(message, "%s is not a valid session ID", (char *)msg.data);
                fill_message(&msg, JN_NAK, strlen(message),
                             (char *)msg.source, message);
            }
            else if (session_to_join == NULL){
                char message [MAX_DATA] = "session does not exist";
                sprintf(message, "session #%s is not a valid session ID", (char *)msg.data);
                fill_message(&msg, JN_NAK, strlen(message),
                             (char *)msg.source, message);
            }
            else{
                int index = return_user_index(name);
                strcpy(user_list[index].session_id, session_to_join->session_id);
                pthread_mutex_lock(&join_mutex);
                session_to_join->user_count++;
                pthread_mutex_unlock(&join_mutex);
                char message [MAX_DATA] = "joined session";
                msg.type = JN_ACK;
            }
        }
        if (msg.type == LEAVE_SESS){
            printf("%s left session\n", msg.source);
            int index = return_user_index(name);
            session_status_t *session_to_leave = session_list_find(list, user_list[index].session_id);
            if (session_to_leave == NULL){
                char message [MAX_DATA] = "you are not in a session";
                fill_message(&msg, msg.type, strlen(message),
                             (char *)msg.source, message);
            }
            char session_id [20];
            strcpy(session_id, session_to_leave->session_id);
            pthread_mutex_lock(&leave_mutex);
            session_to_leave->user_count--;
            printf("there is %d people in the session", session_to_leave->user_count);
            if (session_to_leave->user_count == 0){
                printf("session #%s taken down\n", session_to_leave->session_id);
                session_list_remove(list, session_to_leave);
            }
            strcpy(user_list[index].session_id, NOT_IN_SESSION);
            pthread_mutex_unlock(&leave_mutex);
            char message [50] = "left session";
            sprintf(message, "You have left session #%s\n", session_id);
            strcpy((char *)msg.data, message);
            msg.size = strlen(message);
            fill_message(&msg, msg.type, strlen(message),
                         (char *)msg.source, message);
        }
        if (msg.type == MESSAGE){
            session_status_t * message_session = session_list_find(list, user_list[return_user_index(name)].session_id);
            if(message_session == NULL){
                char message [MAX_DATA] = "you are not in a session";
                fill_message(&msg, MESSAGE, strlen(message),
                             (char *)msg.source, message);
            }
            else{
                send_message_in_a_session(&msg,message_session->session_id);
            }
        }

        puts("sending a message");
        message_to_buffer(&msg, data);
        bytes_sent = send(socekt_fd, data, ACC_BUFFER_SIZE, 0);
        printf("bytes sent: %zd\n",bytes_sent);
        printf("the message is %s\n", msg.data);
        check = error_check((int) bytes_sent, SEND_ERROR,
                            "failed to send the message");
        if (authentication != SUCCESS_LOGIN) break;
        if (!check) break;
    }
    close(socekt_fd);
    //pthread_exit(NULL);
    puts("exited the thread");
    return NULL;
}

void generate_login_response(int authentication, char* username, message_t* msg,
                             unsigned char* buffer ,int buffer_size){
    memset(buffer, 0, buffer_size);
    memset(msg->data, 0, MAX_DATA);

    if (authentication == SUCCESS_LOGIN) {
        msg->type = LO_ACK;
        msg->size = 0;
        message_to_buffer(msg, buffer);
    } else {
        msg->type = LO_NAK;
        msg->size = strlen(login_error_types[authentication]);
        printf("the message size is %d\n", msg->size);
        strcpy((char *) msg->data, login_error_types[authentication]);
        fill_message(msg, LO_NAK, strlen(login_error_types[authentication]),
                     username, login_error_types[authentication]);
        message_to_buffer(msg, buffer);
        printf("message content is %s\n", (char *)msg->data);
        printf("buffer content is %s\n", buffer);
    }
};

void get_active_user_list(message_t * msg) {
    char session_id[MAX_SESSION_CAPACITY];
    char header[256];
    char content[32];
    memset(msg->data, 0, MAX_DATA);
    strcat((char *)msg->data, "active users:\n");
    sprintf(header, "%-17s %s\n", "username", "session_id");
    strcat((char *)msg->data, header);
    for(int i = 0; i < user_count; ++i) {
        if (user_list[i].status == ONLINE) {
            memset(content, 0, 32);
            if (strcmp(user_list[i].session_id, NOT_IN_SESSION) == 0) {
                sprintf(content, "%-17s %s\n",
                        (char *) user_list[i].username, "not in session");
            } else {
                sprintf(content, "%-17s %s\n",
                        (char *) user_list[i].username, user_list[i].session_id);
            }
            strcat((char *)msg->data, content);
        }
    }
    printf("%s", (char *)msg->data);
    msg->size = strlen((char *)msg->data);
};

int return_user_index(char * username){
    for (int i = 0; i < user_count; ++i) {
        if (strcmp((char *)user_list[i].username, (char *)username) == 0){
            return i;
        }
    }
    return -1;
}

void send_message_in_a_session(message_t * msg, char * session_id){
    char buffer [ACC_BUFFER_SIZE];
    message_to_buffer(msg, (unsigned char *)buffer);
    for (int i = 0; i < user_count; ++i) {
        if (strcmp(user_list[i].session_id, session_id) == 0){
            if (return_user_index((char *)msg->source) == i) continue;
            if (user_list[i].socket_fd == OFFLINE) continue;
            printf("sending message to %s\n", (char *)user_list[i].username);
            ssize_t bytes = send(user_list[i].socket_fd, buffer, ACC_BUFFER_SIZE, 0);
            if (bytes == 0){
                printf("connection to %s was closed", (char *)user_list[i].username);
            }
            if (bytes == -1){
                printf("failed to send message to %s\n", (char *)user_list[i].username);
            }
        }
    }
}
