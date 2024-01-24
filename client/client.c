#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int connection_status = 0;
int session_status = 0;
int terminate_signal = 0;
char name[MAX_NAME];


void* server_message_handler(void* socket_fd){
    while(connection_status == OFF);
    int server_reply_socket = *((int*)socket_fd);
    size_t bytes_received;
    char buffer[maximum_buffer_size];
    message_t message;

    while (1){
        if (terminate_signal == ON) break;
        memset(buffer, 0, sizeof(buffer));
        memset(&message, 0, sizeof(message));
        memset(&bytes_received, 0, sizeof(bytes_received));
        bytes_received = receive_message(&server_reply_socket, buffer);
        if (bytes_received == RECEIVE_ERROR) {
            fprintf(stderr, "Error receiving message\n");
            break;
        }
        else if (bytes_received == CONNECTION_REFUSED) {
            fprintf(stderr, "You've been logged out!\n");
            break;
        }


        string_to_message(&message, buffer);
        if (message.type == LO_ACK){
            strcpy(name, (char *)message.source);
            fprintf(stdout, "%s hopped on the server\n", name);
        }
        if (message.type == LO_NAK){
            fprintf(stderr, "Login failed %s\n", (char *)message.data);
            continue;
        }
        if (message.type == JN_ACK){
            fprintf(stdout, "You joined session %s\n", (char *)message.data);
            session_status = ON;
            continue;
        }
        if (message.type == JN_NAK){
            fprintf(stderr, "Joining session failed %s\n", (char *)message.data);
            continue;
        }
        if (message.type == NS_ACK){
            fprintf(stdout, "You created session %s\n", (char *)message.data);
            session_status = ON;
            continue;
        }
        if (message.type == NEW_SESS){
            fprintf(stderr, "Failed to create session %s \n", (char *)message.data);
            continue;
        }
        if (message.type == MESSAGE && strcmp((char *)message.source, name) != 0){
            fprintf(stdout, "%s: %s\n", (char *)message.source, (char *)message.data);
            continue;
        }
        if (message.type == QU_ACK){
            fprintf(stdout, "%s", (char *)message.data);
            continue;
        }

    }
    return NULL;
}


int main(){
    fprintf(stdout, "Welcome to the chatroom!\n");
    char * line_buffer = NULL;
    size_t line_buffer_size;
    ssize_t bytes_read;
    char input[maximum_parameter_size][20];
    int socket_fd;
    pthread_t thread;

    while(ON){
        //reset the input array
        for (int i = 0; i < maximum_parameter_size; ++i) {
            memset(input[i], 0, sizeof(input[i]));
        }
        bytes_read = getline(&line_buffer, &line_buffer_size, stdin);
        if (bytes_read == 1 && line_buffer[0] == '\n') {
            continue;
        }
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading input\n");
            continue;
        }

        int index = 0;
        char* token = strtok(line_buffer, " \n\0");
        while(token != NULL){
            if (index == maximum_parameter_size) {
                fprintf(stderr, "Too many arguments\n");
                continue;
            }
            strcpy(input[index], token);
            index++;
            token = strtok(NULL, " \n");
        }

        if (strcmp(input[0], "/quit") == 0) {
            if (index != 1) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }
            terminate_program(&socket_fd);
            break;
        }
        if (strcmp(input[0], "/login") == 0) {
            if (index != 5) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }
            char * username = input[1];
            char * password = input[2];
            char * ip_address = input[3];
            char * port = input[4];
            login(&socket_fd, ip_address, port, username, password, &thread);
        }
        //register goes here
        if (connection_status == OFF){
            fprintf(stderr, "Please login first\n");
            continue;
        }
        else if (strcmp(input[0], "/logout") == 0) {
            if (index != 1) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }
            logout(&socket_fd);
        } else if (strcmp(input[0], "/joinsession") == 0) {
            if (index != 2) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }
            char * session_id = input[1];
            join_session(&socket_fd, session_id);
        } else if (strcmp(input[0], "/leavesession") == 0) {
            if (index != 1) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }
            leave_session(&socket_fd);
        } else if (strcmp(input[0], "/createsession") == 0) {
            if (index != 2) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }
            char * session_id = input[1];
            create_session(&socket_fd, session_id);
        } else if (strcmp(input[0], "/list") == 0) {
            if (index != 1) {
                fprintf(stderr, "Invalid number of arguments\n");
                continue;
            }

            list(&socket_fd);
        } else{
            if (session_status == OFF){
                fprintf(stderr, "Please join a session first\n");
                continue;
            }
            char message_data[1000];
            char sending_buffer[maximum_buffer_size];
            strcpy(message_data, line_buffer);
            message_t message;
            fill_message(&message, MESSAGE, strlen(message_data), name, message_data);
            message_to_string(&message,message.size, sending_buffer);
            send_message(&socket_fd, maximum_buffer_size, sending_buffer);
        }


    }
    pthread_join(thread, NULL);
    free(line_buffer);
    return 0;
}

void login(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread){
    // Beej's book
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p; // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((status = getaddrinfo(ip_address, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*socket_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(*socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*socket_fd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    set_connection_status(ON);
    pthread_create(&(*thread), NULL, &server_message_handler, &(*socket_fd));
    //send login message
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, LOGIN, strlen(password),username, password);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd,maximum_buffer_size, buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection error\n");
    }
    freeaddrinfo(servinfo); // all done with this structure

}

void logout(int * socket_fd){
    message_t msg;
    char buffer[maximum_buffer_size];
    if (session_status == ON){
        leave_session(socket_fd);
    }
    set_connection_status(OFF);
    fill_message(&msg, EXIT, 0, name, NULL);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, maximum_buffer_size,buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection error\n");
    }
}

void join_session(int * socket_fd, char * session_id){
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, JOIN, strlen(session_id), name, session_id);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, maximum_buffer_size,buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection error\n");
    }
}

void leave_session(int * socket_fd){
    message_t msg;
    char buffer[maximum_buffer_size];
    session_status = OFF;
    fill_message(&msg, LEAVE_SESS, 0, name, NULL);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, maximum_buffer_size,buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection error\n");
    }
}

void create_session(int * socket_fd, char * session_id){
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, NEW_SESS, strlen(session_id), name, session_id);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, maximum_buffer_size,buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection error\n");
    }
}
void list(int * socket_fd){
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, QUERY, 0, name, NULL);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, maximum_buffer_size,buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection error\n");
    }
}

void terminate_program(int * socket_fd){
    if (connection_status == ON){
        logout(socket_fd);
    }
    terminate_signal = ON;
}


void set_connection_status(int status){
    connection_status = status;
}