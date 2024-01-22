#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int server_connection_status = 0;
char username[MAX_NAME];
char arguments[5][20];
int arguments_size = 0;
int session_status = 0;

int main(){
    char* terminal_buffer = malloc(sizeof(char) * terminal_buffer_size);
    message_t msg;;
    int action_fd;
    ssize_t bytes_sent;
    unsigned char buffer[ACC_BUFFER_SIZE];

    pthread_t thread;
    pthread_create(&thread, NULL, response_handler, (void *) &action_fd);
    while(1){
        take_terminal_input(&terminal_buffer);

        unsigned int type;
        command_to_type(arguments[0],&type);

        if (type == INVALID){
            printf("invalid command\n");
            continue;
        }

        if (type == LOGIN){
            if (arguments_size != 5){
                printf("invalid number of arguments\n");
                continue;
            }
            if (server_connection_status == 1){
                printf("you are already logged in\n");
                continue;
            }
            connect_to_server(arguments[3], arguments[4], &action_fd);
            memset(username, 0, MAX_NAME);
            strcpy(username, arguments[1]);
            fill_message(&msg, LOGIN, sizeof(arguments[2]), username, arguments[2]);
            send_a_message(&action_fd, &msg);
            continue;
        }
        if(type == EXIT){
            if (arguments_size != 1){
                printf("invalid number of arguments\n");
                continue;
            }
            if(server_connection_status == 0){
                free(terminal_buffer);
                terminal_buffer = NULL;
                puts("exiting the program");
                puts("goodbye!");
                break;
            }
            puts("sending message to server");
            fill_message(&msg, EXIT, 0, username, NULL);
            send_a_message(&action_fd, &msg);
            printf("closing connection\n");
            close(action_fd);
            server_connection_status = 0;
            free(terminal_buffer);
            terminal_buffer = NULL;
            printf("connection closed\n");
        }

        if(server_connection_status == 0){
            printf("Please connect to the server first!\n");
            continue;
        }
        if (type == LOGOUT){
            if (arguments_size != 1){
                printf("invalid number of arguments\n");
                continue;
            }
            puts("sending message to server");
            fill_message(&msg, EXIT, 0, username, NULL);
            send_a_message(&action_fd, &msg);
            printf("closing connection\n");
            close(action_fd);
            server_connection_status = 0;
            printf("connection closed\n");
            continue;
        }

    }

    pthread_exit(NULL);
    return 1;
}

void* response_handler(void* arg){
    while(server_connection_status == 0);
    int fd = *(int*) arg;
    unsigned char buffer[ACC_BUFFER_SIZE];
    ssize_t byte_received;
    message_t msg;
    while (1){
        memset(buffer, 0, ACC_BUFFER_SIZE);
        memset(&msg, 0, sizeof(message_t));
        byte_received = recv(fd, buffer, ACC_BUFFER_SIZE, 0);
        if (byte_received == 0){
            printf("server closed the connection\n");
            close(fd);
            server_connection_status = 0;
            break;
        }
        if(!error_check((int) byte_received, RECEIVE_ERROR,
                        "failed to receive the message from the server")) return NULL;
        buffer_to_message(&msg, buffer);
        printf("Response Type is: %d\n", msg.type);
        printf("Server Message: %s\n", (char *)msg.data);
        printf("============================================\n");
        decode_server_response(msg.type, (char *)msg.data);
    }
    return NULL;
}

void command_to_type(char * command, unsigned int * type) {
    if (strcmp(command, "/login") == 0) {
        *type = LOGIN;
    } else if (strcmp(command, "/logout") == 0) {
        *type = LOGOUT;
    } else if (strcmp(command, "/joinsession") == 0) {
        *type = JOIN;
    } else if (strcmp(command, "/leavesession") == 0) {
        *type = LEAVE_SESS;
    } else if (strcmp(command, "/createsession") == 0) {
        *type = NEW_SESS;
    } else if (strcmp(command, "/list") == 0) {
        *type = QUERY;
    } else if (strcmp(command, "/quit") == 0) {
        *type = EXIT;
    } else {
        *type = INVALID;
    }
//    } else if (strcmp(command, "/invite") == 0){
//        *type = INVITE;
//    } else if (strcmp(command, "/accept") == 0){
//        *type = ACCEPT;
//    } else if (strcmp(command, "/decline") == 0){
//        *type = DECLINE;
//    } else if (strcmp(command, "/message") == 0){
//        *type = MESSAGE;
//    }
};

void connect_to_server(char * ip_address, char * port, int * socket_fd){
    //cited from page 21 of Beej's Guide to Network Programming, with modifications
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    status = getaddrinfo(ip_address, port, &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    //end of citation


    *socket_fd = socket(servinfo->ai_family,
                        servinfo->ai_socktype, servinfo->ai_protocol);
    if(!error_check(*socket_fd, FD_ERROR,
                    "failed to create a socket")) return;

    int condition = connect(*socket_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(!error_check(condition, CONNECT_ERROR,
                    "failed to connect to the server")) return;
    freeaddrinfo(servinfo);
    server_connection_status = 1;
}

void take_terminal_input(char ** terminal_buffer){
    arguments_size = 0;
    memset((*terminal_buffer), 0, terminal_buffer_size);
    size_t buffer_size = terminal_buffer_size-1;
    ssize_t bytes_read = getline(&(*terminal_buffer), &(buffer_size), stdin);
    if (bytes_read == -1){
        perror("failed to read from stdin");
        exit(errno);
    }

    char* content = strtok((*terminal_buffer), " \n");

    int argument_count = 0;

    //setting up TCP socket
    while(content != NULL){
        if (argument_count > 4) {
            printf("too many arguments\n");
            free((*terminal_buffer));
            exit(1);
        };
        strcpy(arguments[argument_count], content);
        content = strtok(NULL, " \0");
        if (content == NULL){
            unsigned long len = strlen(arguments[argument_count]);
            if (arguments[argument_count][len-1] == '\n'){
                arguments[argument_count][len-1] = '\0';
            }
        }
        argument_count++;
    }
    arguments_size = argument_count;
}

void send_a_message(int *fd, message_t * msg){
    unsigned char buffer[ACC_BUFFER_SIZE];
    memset(buffer, 0, ACC_BUFFER_SIZE);
    message_to_buffer(msg, buffer);
    ssize_t bytes_sent = send(*fd, buffer, ACC_BUFFER_SIZE, 0);
    if(bytes_sent == 0){
        printf("server closed the connection\n");
        close(*fd);
        server_connection_status = 0;
        session_status = 0;
        return;
    }
    if(!error_check((int) bytes_sent, SEND_ERROR,
                    "failed to send the message to the server")) return;
}