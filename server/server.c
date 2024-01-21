#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../message/message.h"
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
    error_check(code, SETUP_ERROR,
                "failed to set up the user list");

    int listen_fd = socket(servinfo->ai_family,
                           servinfo->ai_socktype, servinfo->ai_protocol);
    error_check(listen_fd, FD_ERROR,
                "failed to create a socket");

    int condition = bind(listen_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    error_check(condition, BIND_ERROR,
                "failed to bind the socket to the specified address");

    condition = listen(listen_fd, CONNECTION_CAPACITY);
    error_check(condition, LISTEN_ERROR,
                "failed to listen to the port");

    addr_size = sizeof(their_addr);
    while(1){
        int accept_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &addr_size);
        printf("connection accepted\n");
        error_check(accept_fd, ACCEPT_ERROR,
                    "failed to accept the connection");

        unsigned char data[ACC_BUFFER_SIZE];
        ssize_t bytes_recv;
        ssize_t bytes_sent;

        bytes_recv = recv(accept_fd, data, ACC_BUFFER_SIZE, 0);
        error_check((int)bytes_recv, RECEIVE_ERROR,
                    "failed to accept the connection");
        printf("message received\n");

        struct message msg;
        buffer_to_message(&msg, data);
        printf("type: %d\n", msg.type);
        printf("size: %d\n", msg.size);
        printf("source: %s\n", msg.source);
        printf("data: %s\n", msg.data);

        if (msg.type == LOGIN){
            int login_status = verify_login(msg.source, msg.data);
            printf("login from %s\n", msg.source);
            unsigned char message[ACC_BUFFER_SIZE];
            if (login_status == SUCCESS_LOGIN){
                msg.type = LO_ACK;
                msg.size = 0;
                memset(msg.data, 0, MAX_DATA);
                message_to_buffer(&msg, message);
                bytes_sent = send(accept_fd, message, ACC_BUFFER_SIZE, 0);
                error_check((int)bytes_sent, SEND_ERROR,
                            "failed to send the message");
            } else {
                msg.type = LO_NAK;
                msg.size = 0;
                memset(msg.data, 0, MAX_DATA);
                strcpy((char *)msg.data, login_error_types[login_status]);
                message_to_buffer(&msg, message);
                bytes_sent = send(accept_fd, message, ACC_BUFFER_SIZE, 0);
                error_check((int)bytes_sent, SEND_ERROR,
                            "failed to send the message");
            }
        }
    }
    freeaddrinfo(servinfo);
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