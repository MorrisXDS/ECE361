#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int connection_status = 0;

int main(int argc, char* argv[]){
    char* terminal_buffer = malloc(sizeof(char) * terminal_buffer_size);
    memset(terminal_buffer, 0, terminal_buffer_size);
    size_t buffer_size = terminal_buffer_size-1;
    ssize_t bytes_read = getline(&terminal_buffer, &(buffer_size), stdin);
    if (bytes_read == -1){
        perror("failed to read from stdin");
        exit(errno);
    }

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
    connection_status = 1;

    unsigned char buffer[ACC_BUFFER_SIZE];
    message_t msg;
    msg.type = LOGIN;
    strcpy((char *)msg.data, arguments[2]);
    msg.size = strlen((char *)msg.data);
    strcpy((char *)msg.source, arguments[1]);
    message_to_buffer(&msg, buffer);
    ssize_t bytes_sent;
    printf("============================================\n");
    printf("message being sent to server: %s\n", (char *)msg.data);
    printf("sending message to server\n");
    bytes_sent = send(action_fd, buffer, ACC_BUFFER_SIZE, 0);
    error_check((int) bytes_sent, SEND_ERROR,
                "failed to send the message to the server");

    ssize_t byte_received;
    memset(buffer, 0, ACC_BUFFER_SIZE);
    memset(&msg, 0, sizeof(message_t));
    byte_received = recv(action_fd, buffer, ACC_BUFFER_SIZE, 0);
    error_check((int) byte_received, RECEIVE_ERROR,
                "failed to receive the message from the server");
    buffer_to_message(&msg, buffer);

    printf("Response Type is: %d\n", msg.type);
    printf("============================================\n");
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
            token = strtok(NULL, " \0");
            argument_count++;
        }

        command = check_command(arguments[0]);
        if (command == -1) {
            printf("invalid command\n");
            free(terminal_buffer);
            exit(1);
        }
        else if( command != 0 && connection_status == 0){
            printf("Please connect to the server first!\n");
            continue;
        }

        command_to_type(commands[0], &msg);
        if (msg.type == 0){
            printf("you're logging out of the server\n");
            close(action_fd);
            connection_status = 0;
            printf("connection closed\n");
            continue;
        }

        printf("sending message to server\n");
        // printf("buffer content: %s\n", (char *)buffer);
        bytes_sent = send(action_fd, buffer, ACC_BUFFER_SIZE, 0);
        error_check((int) bytes_sent, SEND_ERROR,
                    "failed to send the message to the server");

        memset(buffer, 0, ACC_BUFFER_SIZE);
        memset(&msg, 0, sizeof(message_t));
        byte_received = recv(action_fd, buffer, ACC_BUFFER_SIZE, 0);
        error_check((int) byte_received, RECEIVE_ERROR,
                    "failed to receive the message from the server");
        buffer_to_message(&msg, buffer);

        printf("Response Type is: %d\n", msg.type);
        decode_server_response(msg.type, (char *)msg.data);
    }

    //close(action_fd);
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
    message_t msg;
    while (1){
        memset(buffer, 0, ACC_BUFFER_SIZE);
        memset(&msg, 0, sizeof(message_t));
        byte_received = recv(fd, buffer, ACC_BUFFER_SIZE, 0);
        error_check((int) byte_received, RECEIVE_ERROR,
                    "failed to receive the message from the server");
        buffer_to_message(&msg, buffer);
        printf("Response Type is: %d\n", msg.type);
        decode_server_response(msg.type, (char *)msg.data);
    }
}

void command_to_type(char * command, message_t * msg){
    if (strcmp(command, "/login") == 0){
        msg->type = LOGIN;
    } else if (strcmp(command, "/logout") == 0){
        msg->type = 0;
    } else if (strcmp(command, "/joinsession") == 0){
        msg->type = JOIN;
    } else if (strcmp(command, "/leavesession") == 0){
        msg->type = LEAVE_SESS;
    } else if (strcmp(command, "/createsession") == 0){
        msg->type = NEW_SESS;
    } else if (strcmp(command, "/list") == 0){
        msg->type = QUERY;
    } else if (strcmp(command, "/quit") == 0){
        msg->type = EXIT;
    } else {
        msg->type = INVALID;
    }
//    } else if (strcmp(command, "/invite") == 0){
//        msg->type = INVITE;
//    } else if (strcmp(command, "/accept") == 0){
//        msg->type = ACCEPT;
//    } else if (strcmp(command, "/decline") == 0){
//        msg->type = DECLINE;
//    } else if (strcmp(command, "/message") == 0){
//        msg->type = MESSAGE;
//    }
}