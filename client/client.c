#include "client.h"
#include <stdio.h>
#include <pthread.h>

int server_connection_status = OFFLINE;
int verified_access = OFF;
int session_status = OFFLINE;
int role = USER;
int termination_signal = OFF;
char name[MAX_NAME];

void* server_message_handler(void* socket_fd){
    int server_reply_socket = *((int*)socket_fd);
    size_t bytes_received;
    char buffer[maximum_buffer_size];
    message_t message;

    while (server_connection_status == ONLINE){
        if (termination_signal == ON) break;
        memset(buffer, 0, sizeof(buffer));
        memset(&message, 0, sizeof(message));
        memset(&bytes_received, 0, sizeof(bytes_received));
        bytes_received = receive_message(&server_reply_socket, buffer);
        if (bytes_received == RECEIVE_ERROR) {
            fprintf(stderr, "Error receiving message Server was likely crashed!\n");
            if (server_connection_status == ONLINE){
                logout(&server_reply_socket);
            }
            break;
        }
        else if (bytes_received == CONNECTION_REFUSED) {
            fprintf(stderr, "You've been logged out!\n");
            if (server_connection_status == ONLINE){
                logout(&server_reply_socket);
            }
            break;
        }

        string_to_message(&message, buffer);
        if (message.type == MESSAGE ){
            if (strcmp((char *)message.source, name) == 0)
                continue;
            fprintf(stdout, "%s: %s\n", (char *)message.source, (char *)message.data);
            continue;
        }
        fprintf(stdout, "===============================\n");
        if (message.type == LO_ACK){
            strcpy(name, (char *)message.source);
            fprintf(stdout, "%s hopped on the server\n", name);
            verified_access = ON;
        }
        if (message.type == LO_NAK){
            fprintf(stderr, "Login failed. %s\n", (char *)message.data);
            verified_access = OFF;
            server_connection_status = OFFLINE;
            set_server_connection_status(OFFLINE);
            break;
        }
        if (message.type == JN_ACK){
            role = USER;
            fprintf(stdout, "You joined session %s\n", (char *)message.data);
            session_status = ONLINE;
        }
        if (message.type == JN_NAK){
            role = USER;
            fprintf(stderr, "Joining session failed. %s\n", (char *)message.data);
            session_status = OFFLINE;
        }
        if (message.type == NS_ACK){
            role = ADMIN;
            fprintf(stdout, "You created session %s\nYou've been promoted to an admin\n", (char *)message.data);
            session_status = ONLINE;
        }
        if (message.type == NEW_SESS){
            fprintf(stderr, "Failed to create session. %s\n", (char *)message.data);
        }
        if (message.type == QU_ACK){
            fprintf(stdout, "%s", (char *)message.data);
        }
        if (message.type == RG_ACK){
            strcpy(name, (char *)message.source);
            fprintf(stdout, "You have become a registered user on TCL!\n");
            verified_access = ON;
        }
        if (message.type == RG_NAK){
            fprintf(stderr, "Registration failed. %s\n", (char *)message.data);
            verified_access = OFF;
            server_connection_status = OFFLINE;
            break;
        }
        if (message.type == MESSAGE){
            fprintf(stdout, "%s: %s\n", (char *)message.source, (char *)message.data);
        }
        if (message.type == PM_MESSAGE){
            role = ADMIN;
            fprintf(stdout, "You have been given the Admin role.\n");
        }
        if (message.type == PM_ACK){
            role = USER;
            fprintf(stdout, "You have been re-assigned the USER role.\n"
                            "Admin Access is now transferred to %s.\n", (char *)message.data);
        }
        if (message.type == PM_NAK){
            fprintf(stderr, "User Access Promotion Failed.\n %s\n", (char *)message.data);
        }
        if (message.type == KI_MESSAGE){
            session_status = OFFLINE;
            fprintf(stdout, "You have been removed from session \"%s\".\n", (char*) message.data);
        }
        if (message.type == KI_ACK){
            fprintf(stdout, "Action Permitted.\n USER %s hos now been kicked out!\n", (char *)message.data);
        }
        if (message.type == KI_NAK){
            fprintf(stderr, "Removing user failed.\n%s\n", (char *)message.data);
        }
        if (message.type == SQ_ACK){
            fprintf(stdout, "%s", (char*) message.data);
        }
        if (message.type == SQ_NAK){
            fprintf(stderr, "Failed to query the session.\n%s\n", (char *)message.data);
        }
        fprintf(stdout, "===============================\n");

    }
    close(server_reply_socket);
    return NULL;
}


int main(){
    fprintf(stdout, "Welcome to the TCL!\n");
    char * line_buffer = NULL;
    size_t line_buffer_size;
    ssize_t bytes_read;
    char input[login_parameter_size][20];
    int socket_fd;
    pthread_t thread[thread_number];
    int thread_count = 0;

    while(ON){
        //reset the input array
        if (thread_count == (thread_number-1)) {
            for (int i = 0; i < thread_count; ++i) {
                pthread_join(thread[i], NULL);
            }
            fprintf(stderr, "Login Attempts used up Please restart the program if you would like to try again.\n");
            return 0;
        }
        for (int i = 0; i < login_parameter_size; ++i) {
            memset(input[i], 0, sizeof(input[i]));
        }
        bytes_read = getline(&line_buffer, &line_buffer_size, stdin);
        if (bytes_read == 0) {
            continue;
        }
        if (bytes_read == -1) {
            fprintf(stderr, "Error reading input\n");
            continue;
        }
        else if (line_buffer[0] =='\n') {
            continue;
        }

        int index = 0;
        char* token = strtok(line_buffer, " \n\0");
        while(token != NULL){
            if (index == login_parameter_size) {
                index++;
                break;
            }
            strcpy(input[index], token);
            if (line_buffer[0] !='/') {
                break;
            }
            index++;
            token = strtok(NULL, " \n");
        }

        if (strcmp(input[0], "/quit") == 0) {
            if (!parameter_count_validate(index, quit_parameter_size)) continue;
            terminate_program(&socket_fd);
            break;
        }
        if (strcmp(input[0], "/login") == 0) {
            if (!parameter_count_validate(index, login_parameter_size)) continue;
            if (verified_access == ON){
                if (strcmp(input[1], name) == 0){
                    fprintf(stderr, "You are already logged in\n");
                    continue;
                }
                else
                    fprintf(stderr, "You cannot login to another account while staying connected\n");
                continue;
            }
            char * username = input[1];
            char * password = input[2];
            char * ip_address = input[3];
            char * port = input[4];
            login(&socket_fd, ip_address, port, username, password, &thread[thread_count]);
            thread_count++;
            continue;
        }
        if (strcmp(input[0], "/create") == 0) {
            if (!parameter_count_validate(index, create_parameter_size)) continue;
            if (verified_access == ON){
                if (strcmp(input[1], name) == 0){
                    fprintf(stderr, "You are already logged in\n");
                    continue;
                }
                else
                    fprintf(stderr, "You cannot login to another account while staying connected\n");
                continue;
            }
            char * username = input[1];
            char * password = input[2];
            char * ip_address = input[3];
            char * port = input[4];
            user_registration(&socket_fd, ip_address, port, username, password, &thread[thread_count]);
            thread_count++;
            continue;
        }
        if (server_connection_status == OFFLINE){
            fprintf(stderr, "Please login first\n");
            continue;
        }
        if (strcmp(input[0], "/logout") == 0) {
            if (!parameter_count_validate(index, logout_parameter_size)) continue;
            logout(&socket_fd);
            continue;
        }
        if (strcmp(input[0], "/list") == 0) {
            if (!parameter_count_validate(index, list_parameter_size)) continue;
            list(&socket_fd);
            continue;
        }
        if (strcmp(input[0], "/userlist") == 0) {
            if (!parameter_count_validate(index, userlist_parameter_size)) continue;
            user_list(&socket_fd, input[1]);
            continue;
        }
        if (strcmp(input[0], "/joinsession") == 0) {
            if (!parameter_count_validate(index, join_session_parameter_size)) continue;
            if (!length_validate((int)strlen(input[1]), MAX_SESSION_LENGTH)) continue;
            char * session_id = input[1];
            join_session(&socket_fd, session_id);
            continue;
        }
        if (strcmp(input[0], "/createsession") == 0) {
            if (!parameter_count_validate(index, create_session_parameter_size)) continue;
            if (!length_validate((int)strlen(input[1]), MAX_SESSION_LENGTH)) continue;
            char * session_id = input[1];
            create_session(&socket_fd, session_id);
            continue;
        }
        if (strcmp(input[0], "/leavesession") == 0) {
            if (!parameter_count_validate(index, leave_session_parameter_size)) continue;
            leave_session(&socket_fd);
            continue;
        }
        if (session_status == ONLINE){
            if (input [0][0] != '/') {
                char sending_buffer[maximum_buffer_size];
                message_t message;
                fill_message(&message, MESSAGE, bytes_read, name, line_buffer);
                message_to_string(&message,message.size, sending_buffer);
                send_message(&socket_fd, strlen(sending_buffer), sending_buffer);
                continue;
            }
            if (role == USER){
                fprintf(stderr, "You are not an admin of the session\n");
                continue;
            }
            if (strcmp(input[0], "/promote") == 0) {
                if (!parameter_count_validate(index, promote_parameter_size)) continue;
                if (!length_validate((int)strlen(input[1]), MAX_NAME)) continue;
                if (strcmp(name, input[1]) == 0){
                    fprintf(stderr, "You cannot promote yourself!\n");
                    continue;
                }
                char * username = input[1];
                promote(&socket_fd, username);
            }
            if (strcmp(input[0],"/kick") == 0) {
                if (!parameter_count_validate(index, kick_parameter_size)) continue;
                if (!length_validate((int)strlen(input[1]), MAX_NAME)) continue;
                if (strcmp(name, input[1]) == 0){
                    fprintf(stderr, "You cannot remove yourself!\n");
                    continue;
                }
                char * username = input[1];
                kick(&socket_fd, username);
            }
            if (input[0][0] == '/'){
                fprintf(stderr, "Invalid Command\n");
                continue;
            }
            continue;
        }
        else if (input[0][0] == '/'){
            fprintf(stderr, "Invalid Command\n");
            continue;
        }
        else{
            fprintf(stderr, "Please join/create a session first!\n");
            continue;
        }


    }

    return 0;
}

void login(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    if (!length_validate((int)strlen(password), MAX_PASSWORD_LENGTH)){
        fprintf(stderr, "Password needs to be under length of %d\n", MAX_PASSWORD_LENGTH);
        return;
    }

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
        freeaddrinfo(servinfo); // free the linked-list
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
        freeaddrinfo(servinfo);
        return;
    }

    set_server_connection_status(ON);
    pthread_create(&(*thread), NULL, &server_message_handler, &(*socket_fd));
    //send login message
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, LOGIN, strlen(password),username, password);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd,strlen(buffer), buffer);
    if(code == 0){
        set_server_connection_status(OFFLINE);
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        set_server_connection_status(OFFLINE);
        printf("Connection lost\n");
    }
    freeaddrinfo(servinfo); // all done with this structure

}

void user_registration(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    if (!length_validate((int)strlen(password), MAX_PASSWORD_LENGTH)){
        fprintf(stderr, "Password needs to be under length of %d\n", MAX_PASSWORD_LENGTH);
        return;
    }

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
        freeaddrinfo(servinfo); // free the linked-list
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
        freeaddrinfo(servinfo);
        return;
    }

    set_server_connection_status(ON);
    pthread_create(&(*thread), NULL, &server_message_handler, &(*socket_fd));
    //send login message
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, CREATE, strlen(password),username, password);
    message_to_string(&msg, msg.size, buffer);
    send_message(socket_fd,strlen(buffer), buffer);
}

void logout(int * socket_fd){
    message_t msg;
    char buffer[maximum_buffer_size];
    if (session_status == ONLINE){
        leave_session(socket_fd);
    }
    verified_access = OFF;
    set_server_connection_status(OFFLINE);
    fill_message(&msg, EXIT, 0, name, NULL);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}

void join_session(int * socket_fd, char * session_id){
    if (session_status == ONLINE){
        fprintf(stderr, "You are already in a session\n");
        return;
    }
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, JOIN, strlen(session_id), name, session_id);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}

void leave_session(int * socket_fd){
    if (session_status == OFFLINE){
        fprintf(stderr, "You are not in a session!\n");
        return;
    }
    message_t msg;
    char buffer[maximum_buffer_size];
    session_status = OFFLINE;
    role = USER;
    fill_message(&msg, LEAVE_SESS, 0, name, NULL);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}

void create_session(int * socket_fd, char * session_id){
    if (session_status == ONLINE){
        fprintf(stderr, "You are already in a session\n");
        return;
    }
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, NEW_SESS, strlen(session_id), name, session_id);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}
void list(int * socket_fd){
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, QUERY, 0, name, NULL);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}

void user_list(int * socket_fd, char * session_id){
    message_t msg;
    char buffer[maximum_buffer_size];
    if(strcmp(session_id, NOT_IN_SESSION) == 0){
        fprintf(stdout,"Session Query Not permitted\n \"not in session\" is not a session");
        return;
    }
    fill_message(&msg, SESS_QUERY, strlen(session_id), name, session_id);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}


void terminate_program(int * socket_fd){
    if (server_connection_status == ON){
        logout(socket_fd);
    }
    termination_signal = ON;
}

void promote(int *socket_fd, char* username){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, PROMOTE, strlen(username), name, username);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}

void kick(int *socket_fd, char* username){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, KICK, strlen(username), name, username);
    message_to_string(&msg, msg.size, buffer);
    unsigned int code = send_message(socket_fd, strlen(buffer),buffer);
    if(code == 0){
        printf("Could Not reach the server at this moment\n");
    }
    else if (code == -1){
        printf("Connection lost\n");
    }
}

void set_server_connection_status(int status){
    server_connection_status = status;
}

int length_validate(int received_length, int expected_length){
    if (received_length >= expected_length){
        printf("Parameter too long\n");
        return validation_failure;
    }
    return validation_success;
}

int parameter_count_validate(int received_count, int expected_count){
    if (received_count  > expected_count){
        printf("Too many parameters\n");
        return validation_failure;
    }
    else if (received_count < expected_count){
        printf("Too few parameters\n");
        return validation_failure;
    }
    return validation_success;
}



