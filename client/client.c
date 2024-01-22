#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int server_connection_status = 0;
char username[MAX_NAME];
char arguments[5][20];
int arguments_size = 0;
char message[MAX_DATA];
int thread_used = 0;
char get_back = 0;
char terminal_session = 0;

int main(){
    char* terminal_buffer = malloc(sizeof(char) * terminal_buffer_size);
    message_t msg;;
    int action_fd;
    ssize_t bytes_sent;
    unsigned char buffer[ACC_BUFFER_SIZE];

    pthread_t thread[thread_capacity];
    while(1){
        if (thread_used == thread_capacity){
            puts("You've logged out too many times, please restart the program");
            break;
        }
        if(get_back == 1){
            get_back = 0;
            continue;
        }
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
            fill_message(&msg, LOGIN, sizeof(arguments[2]), arguments[1], arguments[2]);
            send_a_message(&action_fd, &msg);
            pthread_create(&thread[thread_used], NULL, response_handler, (void *) &action_fd);
            thread_used++;
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
            fill_message(&msg, EXIT, 0, username, NULL);
            send_a_message(&action_fd, &msg);
            close(action_fd);
            server_connection_status = 0;
            free(terminal_buffer);
            terminal_buffer = NULL;
            printf("connection closed\n");
        }
        if (type == REGISTER){
            if (arguments_size != 5){
                printf("invalid number of arguments\n");
                continue;
            }
            connect_to_server(arguments[3], arguments[4], &action_fd);
            fill_message(&msg, REGISTER, sizeof(arguments[2]), arguments[1], arguments[2]);
            send_a_message(&action_fd, &msg);
            pthread_create(&thread[thread_used], NULL, response_handler, (void *) &action_fd);
            thread_used++;
            continue;
        }
        if(server_connection_status == 0){
            printf("Please connect to the server first!\n");
            continue;
        }
        if (type == NEW_SESS){
            if (arguments_size != 2){
                printf("invalid number of arguments\n");
                continue;
            }
            fill_message(&msg, NEW_SESS, sizeof(arguments[1]), username, arguments[1]);
            send_a_message(&action_fd, &msg);
            continue;
        }
        if (type == JOIN){
            if (arguments_size != 2){
                printf("invalid number of arguments\n");
                continue;
            }
            fill_message(&msg, JOIN, sizeof(arguments[1]), username, arguments[1]);
            send_a_message(&action_fd, &msg);
            continue;
        }
        if (type == LEAVE_SESS){
            if (arguments_size != 1){
                printf("invalid number of arguments\n");
                continue;
            }
            fill_message(&msg, LEAVE_SESS, 0, username, NULL);
            send_a_message(&action_fd, &msg);
            continue;
        }
        if (type == LOGOUT){
            if (arguments_size != 1){
                printf("invalid number of arguments\n");
                continue;
            }
            fill_message(&msg, EXIT, 0, username, NULL);
            send_a_message(&action_fd, &msg);
            close(action_fd);
            server_connection_status = 0;
            printf("connection closed\n");
            continue;
        }
        if (type == MESSAGE){
            fill_message(&msg, MESSAGE, sizeof(message), username, message);
            send_a_message(&action_fd, &msg);
            continue;
        }
        if (type == QUERY){
            if (arguments_size != 1){
                printf("invalid number of arguments\n");
                continue;
            }
            fill_message(&msg, QUERY, 0, username, NULL);
            send_a_message(&action_fd, &msg);
            continue;
        }
    }

    pthread_exit(NULL);
    return 1;
}

void* response_handler(void* arg){
    while (server_connection_status == 0);
    int fd = *(int*) arg;
    unsigned char buffer[ACC_BUFFER_SIZE];
    ssize_t byte_received;
    message_t msg;
    while (server_connection_status != 0){
        memset(buffer, 0, ACC_BUFFER_SIZE);
        memset(&msg, 0, sizeof(message_t));
        byte_received = recv(fd, buffer, ACC_BUFFER_SIZE, 0);
        if (byte_received == 0){
            printf("server closed the connection\n");
            close(fd);
            server_connection_status = 0;
            continue;
        }
        if(!error_check((int) byte_received, RECEIVE_ERROR,
                        "failed to receive the message from the server")) return NULL;
        buffer_to_message(&msg, buffer);
        if (msg.type == LO_ACK){
            memset(username, 0, MAX_NAME);
            strcpy(username, (char *)msg.source);
            continue;
        }
        if (msg.type == LO_NAK){
            printf("server closed the connection\n");
            decode_server_response(msg.type, (char *)msg.data);
            close(fd);
            server_connection_status = 0;
        }

        if (msg.type == RG_ACK){
            memset(username, 0, MAX_NAME);
            strcpy(username, (char *)msg.source);
            continue;
        }
        if (msg.type == MESSAGE){
            char display[ACC_BUFFER_SIZE];
            sprintf(display, "%s: %s", (char *)msg.source, (char *)msg.data);
            decode_server_response(msg.type, display);
            continue;
        }
        else if(msg.type == RG_ACK) {
            server_connection_status = 1;
            decode_server_response(msg.type, (char *)msg.data);
        }
        
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
    }
    else if (strcmp(command,"/register") == 0){
        *type = REGISTER;
    }
    else {
        if (command[0] == '/')
            *type = INVALID;
        else
            *type = MESSAGE;
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
    printf("you have been connected to the server\n");
    freeaddrinfo(servinfo);
    server_connection_status = 1;
}

void take_terminal_input(char ** terminal_buffer){
    terminal_session = 1;
    arguments_size = 0;
    memset((*terminal_buffer), 0, terminal_buffer_size);
    size_t buffer_size = terminal_buffer_size-1;
    ssize_t bytes_read = getline(&(*terminal_buffer), &(buffer_size), stdin);
    if (bytes_read == -1){
        perror("failed to read from stdin");
        terminal_session = 0;
        exit(errno);
    }

    if( *terminal_buffer[0] !=  '/'){
        memset(message, 0, MAX_DATA);
        strcpy(message, *terminal_buffer);
        strcpy(arguments[0], "message");
        arguments_size = 1;
        terminal_session = 0;
        return;
    }

    char* content = strtok((*terminal_buffer), " \n");

    int argument_count = 0;

    //setting up TCP socket
    while(content != NULL){
        if (argument_count > 4) {
            printf("too many arguments\n");
            get_back = 1;
            terminal_session = 0;
            return;
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
    terminal_session = 0;
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
        return;
    }
    if(!error_check((int) bytes_sent, SEND_ERROR,
                    "failed to send the message to the server")) return;
}