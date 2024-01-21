#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "server.h"

struct user user_list[LIST_CAPACITY];
int user_count = 0;

int main(int argc, char* argv[]){
    //cited from Page 21 and Page 28, Beej's Guide to Network Programming, with modifications
    int status;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results

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
        puts("accepting connections...\n");
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
        pthread_join(thread_pool[thread_count], NULL);
        thread_count++;
    }
    //wait for other threads to finish before exiting
    freeaddrinfo(servinfo);
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
        user_count++;
    }
    fclose(fp);
    return 1;
}

int verify_login(unsigned char * username, unsigned char * password){
    for (int i = 0; i < user_count; ++i) {
        if (strcmp((char *)user_list[i].username, (char *)username) == 0){
            if (strcmp((char *)user_list[i].password, (char *)password) == 0){
                return SUCCESS_LOGIN;
            } else {
                return LOGIN_ERROR;
            }
        }
    }
    return LOGIN_ERROR;
}

void* connection_handler(void* accept_fd){
    int socekt_fd = *(int *)accept_fd;

    unsigned char data[ACC_BUFFER_SIZE];

    while(1){
        ssize_t bytes_recv = -1;
        ssize_t bytes_sent = -1;
        memset(data, 0, ACC_BUFFER_SIZE);
        puts("waiting for a message");
        bytes_recv = recv(socekt_fd, data, ACC_BUFFER_SIZE, 0);
        printf("bytes received: %zd\n",bytes_recv);
        if (bytes_recv == 0) {
            puts("client disconnected");
            puts("closing the socket");
            close(socekt_fd);
            puts("exiting the thread");
            break;
        }
        int check = error_check((int)bytes_recv, RECEIVE_ERROR,
                                "failed to receive the message");
        if (!check) break;

        message_t msg;
        buffer_to_message(&msg, data);


        if(msg.type == LOGIN) {
            int login_status = verify_login(msg.source, msg.data);
            generate_login_response(login_status, &msg, data, ACC_BUFFER_SIZE);
        }
        if(msg.type == EXIT){
            close(socekt_fd);
            printf("%s closed connection\n", msg.source);
            puts("server ending communication\n");
            break;
        }

        puts("sending a message");
        bytes_sent = send(socekt_fd, data, ACC_BUFFER_SIZE, 0);
        printf("bytes sent: %zd\n",bytes_sent);
        check = error_check((int) bytes_sent, SEND_ERROR,
                            "failed to send the message");
        if (!check) break;
    }

    //pthread_exit(NULL);
    puts("exited the thread");
    return NULL;
}

void generate_login_response(int login_status, message_t* msg, unsigned char* buffer, int buffer_size){
    memset(buffer, 0, buffer_size);
    memset(msg->data, 0, MAX_DATA);

    if (login_status == SUCCESS_LOGIN) {
        msg->type = LO_ACK;
        msg->size = 0;
        message_to_buffer(msg, buffer);
    } else {
        msg->type = LO_NAK;
        msg->size = 0;
        strcpy((char *) msg->data, login_error_types[login_status]);
        message_to_buffer(msg, buffer);
    }
};
