#include "client.h"
#include "../message/message.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


int main(int argc, char* argv[]){
    char* terminal_buffer = malloc(sizeof(char) * terminal_buffer_size);
    memset(terminal_buffer, 0, terminal_buffer_size);
    size_t buffer_size = terminal_buffer_size-1;
    ssize_t bytes_read = getline(&terminal_buffer, &(buffer_size), stdin);
    if (bytes_read == -1){
        perror("failed to read from stdin");
        exit(errno);
    }
    printf("%s", terminal_buffer);

    char* content = strtok(terminal_buffer, " \0");
    char arguments[5][20];

    //setting up TCP socket
    int argument_count = 0;
    while(content != NULL){
        if (argument_count > 4) {
            printf("too many arguments\n");
            free(terminal_buffer);
            exit(1);
        };
        strcpy(arguments[argument_count], content);
        printf("%s\t", arguments[argument_count]);
        content = strtok(NULL, " \0");
        argument_count++;
    }

    int command = check_command(arguments[0]);
    if (command == -1) {
        printf("invalid command\n");
        free(terminal_buffer);
        exit(1);
    }

    //print all arguments out
    for (int i = 0; i < argument_count; i++) {
        printf("argument %d: %s\n",i, arguments[i]);
    }

    puts(arguments[4]);

    int port = atoi(arguments[4]);
    printf("port: %d\n", port);
    sprintf(arguments[4], "%d", port);

    //cited from page 21 of Beej's Guide to Network Programming, with modifications
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    status = getaddrinfo(arguments[3], arguments[4], &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    //end of citation


    int action_fd = socket(servinfo->ai_family,
                           servinfo->ai_socktype, servinfo->ai_protocol);
    error_check(action_fd, FD_ERROR,
                "failed to create a socket");

    int condition = connect(action_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    error_check(condition, CONNECT_ERROR,
                "failed to connect to the server");

    unsigned char buffer[ACC_BUFFER_SIZE];
    struct message msg;
    msg.type = LOGIN;
    strcpy((char *)msg.data, arguments[2]);
    msg.size = strlen((char *)msg.data);
    strcpy((char *)msg.source, arguments[1]);
    message_to_buffer(&msg, buffer);
    ssize_t bytes_sent;

    printf("sending message to server\n");
    printf("buffer content: %s\n", (char *)buffer);
    bytes_sent = send(action_fd, buffer, ACC_BUFFER_SIZE, 0);
    error_check((int) bytes_sent, SEND_ERROR,
                "failed to send the message to the server");

    ssize_t byte_received;
    memset(buffer, 0, ACC_BUFFER_SIZE);
    memset(&msg, 0, sizeof(struct message));
    byte_received = recv(action_fd, buffer, ACC_BUFFER_SIZE, 0);
    error_check((int) byte_received, RECEIVE_ERROR,
                "failed to receive the message from the server");
    buffer_to_message(&msg, buffer);

    printf("Response Type is: %d\n", msg.type);
    decode_server_response(msg.type, (char *)msg.data);

    while(1){
        memset(terminal_buffer, 0, terminal_buffer_size);
        for (int i = 0; i < 5; i++) {
            memset(arguments[i], 0, sizeof(arguments[i]));
        }
        buffer_size = terminal_buffer_size-1;
        bytes_read = getline(&terminal_buffer, &(buffer_size), stdin);
        if (bytes_read == -1){
            perror("failed to read from stdin");
            exit(errno);
        }
        printf("%s", terminal_buffer);

        char* token = strtok(terminal_buffer, " \0");

        //setting up TCP socket
        argument_count = 0;
        while(token != NULL){
            if (argument_count > 4) {
                printf("too many arguments\n");
                free(terminal_buffer);
                exit(1);
            };
            strcpy(arguments[argument_count], token);
            printf("%s\t", arguments[argument_count]);
            token = strtok(NULL, " \0");
            argument_count++;
        }

        command = check_command(arguments[0]);
        if (command == -1) {
            printf("invalid command\n");
            free(terminal_buffer);
            exit(1);
        }

        //print all arguments out
        for (int i = 0; i < argument_count; i++) {
            printf("argument %d: %s\n",i, arguments[i]);
        }

        msg.type = LOGIN;
        strcpy((char *)msg.data, arguments[2]);
        msg.size = strlen((char *)msg.data);
        strcpy((char *)msg.source, arguments[1]);
        message_to_buffer(&msg, buffer);

        printf("sending message to server\n");
        printf("buffer content: %s\n", (char *)buffer);
        bytes_sent = send(action_fd, buffer, ACC_BUFFER_SIZE, 0);
        error_check((int) bytes_sent, SEND_ERROR,
                    "failed to send the message to the server");

        memset(buffer, 0, ACC_BUFFER_SIZE);
        memset(&msg, 0, sizeof(struct message));
        byte_received = recv(action_fd, buffer, ACC_BUFFER_SIZE, 0);
        error_check((int) byte_received, RECEIVE_ERROR,
                    "failed to receive the message from the server");
        buffer_to_message(&msg, buffer);

        printf("Response Type is: %d\n", msg.type);
        decode_server_response(msg.type, (char *)msg.data);
    }

    close(action_fd);
    freeaddrinfo(servinfo);
    free(terminal_buffer);
    terminal_buffer = NULL;

    return 1;
}

int check_command(char * command){
    if (command[0] != '/') return message_id;
    for (int i = 0; i < command_number; i++){
        if (strcmp(command, commands[i]) == 0){
            return i;
        }
    }
    return COMMAND_NOT_FOUND;
};

void* response_handler(void* arg){
    int fd = *(int*) arg;
    unsigned char buffer[ACC_BUFFER_SIZE];
    ssize_t byte_received;
    struct message msg;
    while (1){
        memset(buffer, 0, ACC_BUFFER_SIZE);
        memset(&msg, 0, sizeof(struct message));
        byte_received = recv(fd, buffer, ACC_BUFFER_SIZE, 0);
        error_check((int) byte_received, RECEIVE_ERROR,
                    "failed to receive the message from the server");
        buffer_to_message(&msg, buffer);
        printf("Response Type is: %d\n", msg.type);
        decode_server_response(msg.type, (char *)msg.data);
    }
}