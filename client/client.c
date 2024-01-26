#include "client.h"
#include <stdio.h>
#include <pthread.h>

/* This is the client program where we classify user inputs
 * and perform corresponding actions like log in to a server
 * join a session e.t.c while handling server messages concurrently
 * (probably) with the help of threading*/

/* A few things to note:
 * 1. functions alike will likely be commented once only
 * 2. there is still room for optimization, but I am tired :(
 * 3. happy reading code!
 * 4. Have a good one!*/

//flags to indicate the statuses of the client
//and the server connection
int server_connection_status = OFFLINE;
int verified_access = OFF;
int session_status = OFFLINE;
int role = USER;
int termination_signal = OFF;
char name[MAX_NAME];

/*handle messages from the server in a thread*/
/*all server messages are handled here*/
/*status flags will be set here accordingly*/
void* server_message_handler(void* socket_fd){
    // capture the socket file descriptor
    // and initialize some variables
    int server_reply_socket = *((int*)socket_fd);
    size_t bytes_received;
    char buffer[maximum_buffer_size];
    message_t message;

    // only starts executing when connected to the server
    while (server_connection_status == ONLINE){
        // when receiving a termination signal,
        // break out of the loop to exit the thread
        if (termination_signal == ON) break;
        //zero initialize the buffer and message to
        //prevent any garbage data from being sent
        memset(buffer, 0, sizeof(buffer));
        memset(&message, 0, sizeof(message));
        memset(&bytes_received, 0, sizeof(bytes_received));

        // receive a message from the server
        // and error handle cases where the server
        // has crashed or the connection has been refused
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

        // convert the buffer to a message to
        // extract necessary information
        string_to_message(&message, buffer);
        // display the message if it is a message type
        // and the current user is not the sender

        // handling process based on message type
        // generally involves setting status flags
        // and printing related info to the terminal
        if (message.type == MESSAGE ){
            if (strcmp((char *)message.source, name) == 0)
                continue;
            fprintf(stdout, "%s: %s\n", (char *)message.source, (char *)message.data);
            continue;
        }
        // to separate displayable messages data from commands  
        fprintf(stdout, "===============================\n");
        // access verified
        if (message.type == LO_ACK){
            strcpy(name, (char *)message.source);
            fprintf(stdout, "%s hopped on the server\n", name);
            verified_access = ON;
        }
        // access verification failed we close the connection
        if (message.type == LO_NAK){
            fprintf(stderr, "Login failed. %s\n", (char *)message.data);
            verified_access = OFF;
            server_connection_status = OFFLINE;
            set_server_connection_status(OFFLINE);
            break;
        }
        // joined a session successfully
        if (message.type == JN_ACK){
            role = USER;
            fprintf(stdout, "You joined session %s\n", (char *)message.data);
            session_status = ONLINE;
        }
        // failed to join a session
        // data contains the reason for joinsession
        // failure
        if (message.type == JN_NAK){
            role = USER;
            fprintf(stderr, "Joining session failed. %s\n", (char *)message.data);
            session_status = OFFLINE;
        }
        // created a session successfully
        if (message.type == NS_ACK){
            role = ADMIN;
            fprintf(stdout, "You created session %s\nYou've been promoted to an admin\n", (char *)message.data);
            session_status = ONLINE;
        }
        // failed to create a session
        if (message.type == NEW_SESS){
            fprintf(stderr, "Failed to create session. %s\n", (char *)message.data);
        }
        // queried online user list successfully
        if (message.type == QU_ACK){
            fprintf(stdout, "%s", (char *)message.data);
        }
        // managed to create an account
        if (message.type == RG_ACK){
            strcpy(name, (char *)message.source);
            fprintf(stdout, "You have become a registered user on TCL!\n");
            verified_access = ON;
        }
        // failed to create an account
        // data contains the reason for
        // registration failure
        if (message.type == RG_NAK){
            fprintf(stderr, "Registration failed. %s\n", (char *)message.data);
            verified_access = OFF;
            server_connection_status = OFFLINE;
            break;
        }
        // has been promoted to admin
        if (message.type == PM_MESSAGE){
            role = ADMIN;
            fprintf(stdout, "You have been given the Admin role.\n");
        }
        // promoted a user to admin
        // current user loses admin role
        if (message.type == PM_ACK){
            role = USER;
            fprintf(stdout, "You have been re-assigned the USER role.\n"
                            "Admin Access is now transferred to %s.\n", (char *)message.data);
        }
        // failed to promote a user to admin
        // data contains the reason for failure
        if (message.type == PM_NAK){
            fprintf(stderr, "User Access Promotion Failed.\n %s\n", (char *)message.data);
        }
        // current user gets kicked out of a session
        if (message.type == KI_MESSAGE){
            session_status = OFFLINE;
            fprintf(stdout, "You have been removed from session \"%s\".\n", (char*) message.data);
        }
        // kicked a user out of a session successfully
        if (message.type == KI_ACK){
            fprintf(stdout, "Action Permitted.\n USER %s hos now been kicked out!\n", (char *)message.data);
        }
        // failed to kick a user out of a session
        // for reasons contained in data
        if (message.type == KI_NAK){
            fprintf(stderr, "Removing user failed.\n%s\n", (char *)message.data);
        }
        // queried active user list in a session
        // successfully
        if (message.type == SQ_ACK){
            fprintf(stdout, "%s", (char*) message.data);
        }
        // failed to query active user list in a session
        // for reasons contained in data
        if (message.type == SQ_NAK){
            fprintf(stderr, "Failed to query the session.\n%s\n", (char *)message.data);
        }
        // to separate displayable messages data from commands
        fprintf(stdout, "===============================\n");

    }
    close(server_reply_socket);
    return NULL;
}


int main(){
    // initialize variables for reading and storing user inputs
    // and setting up the connection
    fprintf(stdout, "Welcome to the TCL!\n");
    char * line_buffer = NULL;
    size_t line_buffer_size;
    ssize_t bytes_read;
    char input[login_parameter_size][20];
    int socket_fd;
    pthread_t thread[thread_number];
    int thread_count = 0;

    // keep getting input from the user in terminal
    // until the user quits the program in some way
    while(ON){
        // too many re-connections from a single client session
        // release all thread resources and get ready to
        // exit the program
        if (thread_count == (thread_number)) {
            for (int i = 0; i < thread_count; ++i) {
                pthread_join(thread[i], NULL);
            }
            fprintf(stderr, "Login Attempts used up Please restart the program if you would like to try again.\n");
            return 0;
        }
        // memset zero command buffers to prevent garbage values
        for (int i = 0; i < login_parameter_size; ++i) {
            memset(input[i], 0, sizeof(input[i]));
        }
        // read input from the user and react accordingly
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

        // split the terminal line buffer into tokens
        // separated by  ' ', '\n' or '\0'  and store
        // into command buffers
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
        // all commands are requests which might not succeed

        /*actions that do not require a connection to the server*/

        int code = check_commands(input[0]);

        // invalid command
        if (code == invalid_command){
            fprintf(stderr, "Invalid command\n");
            continue;
        }

        // exit the program
        if (code == quit_command) {
            // check if the exact number of parameters are provided
            if (!parameter_count_validate(index, quit_parameter_size)) continue;
            terminate_program(&socket_fd);
            break;
        }
        // login to the server
        if (code == login_command) {
            if (!parameter_count_validate(index, login_parameter_size)) continue;
            // check if the user is already logged in
            if (verified_access == ON){
                // check if the user is trying to log in to the same account
                // while already logged in
                if (strcmp(input[1], name) == 0){
                    fprintf(stderr, "You are already logged in\n");
                    continue;
                }
                // check if the user is trying to log in to a different account
                // while already logged in
                else
                    fprintf(stderr, "You cannot login to another account while staying connected\n");
                continue;
            }
            // try logging in to the server
            char * username = input[1];
            char * password = input[2];
            char * ip_address = input[3];
            char * port = input[4];
            login(&socket_fd, ip_address, port, username, password, &thread[thread_count]);
            thread_count++;
            continue;
        }
        // create a new account
        if (code == create_command) {
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
        /*actions that do require a connection to the server*/

        // group messaging in a session
        if (code == message_command){
            if (session_status == OFFLINE){
                fprintf(stderr, "Please join a session first\n");
                continue;
            }
            char sending_buffer[maximum_buffer_size];
            message_t message;
            fill_message(&message, MESSAGE, bytes_read, name, line_buffer);
            message_to_string(&message,message.size, sending_buffer);
            send_message(&socket_fd, strlen(sending_buffer), sending_buffer);
            continue;
        }

        // logout from the server
        if (code == logout_command) {
            if (!parameter_count_validate(index, logout_parameter_size)) continue;
            logout(&socket_fd);
            continue;
        }
        // list all online usernames and their sessions
        if (code == list_command) {
            if (!parameter_count_validate(index, list_parameter_size)) continue;
            list(&socket_fd);
            continue;
        }
        // list users, their session_ids and their roles
        // (whether they are ann admin or not) for a
        // given session_id
        if (code == userlist_command) {
            if (!parameter_count_validate(index, userlist_parameter_size)) continue;
            user_list(&socket_fd, input[1]);
            continue;
        }
        // join a session
        if (code == join_session_command) {
            if (!parameter_count_validate(index, join_session_parameter_size)) continue;
            // check if the session_id is too long
            if (!length_validate((int)strlen(input[1]), MAX_SESSION_LENGTH)) continue;
            char * session_id = input[1];
            join_session(&socket_fd, session_id);
            continue;
        }
        // create a new session
        if (code == create_session_command) {
            if (!parameter_count_validate(index, create_session_parameter_size)) continue;
            if (!length_validate((int)strlen(input[1]), MAX_SESSION_LENGTH)) continue;
            char * session_id = input[1];
            create_session(&socket_fd, session_id);
            continue;
        }
        // leave a session
        if (code == leave_session_command) {
            if (!parameter_count_validate(index, leave_session_parameter_size)) continue;
            leave_session(&socket_fd);
            continue;
        }
        /* all actions in this if branch require the user
         * to be in a session*/

        if (session_status == OFFLINE){
            /* all actions below this if statement require an ADMIN role*/
            fprintf(stderr, "Please join a session first\n");
            continue;
        }

        if (role == USER){
            fprintf(stderr, "You are not an admin of the session\n");
            continue;
        }

        // promote a user to admin
        // the promoter will be demoted to a user
        if (code == promote_command) {
            if (!parameter_count_validate(index, promote_parameter_size)) continue;
            if (!length_validate((int)strlen(input[1]), MAX_NAME)) continue;
            if (strcmp(name, input[1]) == 0){
                fprintf(stderr, "You cannot promote yourself!\n");
                continue;
            }
            char * username = input[1];
            promote(&socket_fd, username);
        }

        // kick a user from the session that the admin is in
        if (code == kick_command) {
            if (!parameter_count_validate(index, kick_parameter_size)) continue;
            if (!length_validate((int)strlen(input[1]), MAX_NAME)) continue;
            if (strcmp(name, input[1]) == 0){
                fprintf(stderr, "You cannot remove yourself!\n");
                continue;
            }
            char * username = input[1];
            kick(&socket_fd, username);
        }
    }
    return 0;
}

/* Validate inputs, set up the socket, connect
 and send a login request to the server*/
void login(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread){
    /* validate the lengths of username and password*/
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    if (!length_validate((int)strlen(password), MAX_PASSWORD_LENGTH)){
        fprintf(stderr, "Password needs to be under length of %d\n", MAX_PASSWORD_LENGTH);
        return;
    }

    // cited from Beej's Guide to Network Programming
    // set up the socket and connect to the server
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
    // end of citation

    // connection established
    set_server_connection_status(ON);
    // create a thread to handle server messages
    pthread_create(&(*thread), NULL, &server_message_handler, &(*socket_fd));
    // prepare and send login message
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

/* Validate inputs, set up the socket, connect to the server
 and send a registration request to the server
 it works like the login function... no further comments*/
void user_registration(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    if (!length_validate((int)strlen(password), MAX_PASSWORD_LENGTH)){
        fprintf(stderr, "Password needs to be under length of %d\n", MAX_PASSWORD_LENGTH);
        return;
    }

    // cited from Beej's Guide to Network Programming
    // set up the socket and connect to the server
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
    // end of citation

    set_server_connection_status(ON);
    pthread_create(&(*thread), NULL, &server_message_handler, &(*socket_fd));
    //send login message
    message_t msg;
    char buffer[maximum_buffer_size];
    fill_message(&msg, CREATE, strlen(password),username, password);
    message_to_string(&msg, msg.size, buffer);
    send_message(socket_fd,strlen(buffer), buffer);
}

/* logout from the server, leave session if in a session
 set related flags similar to a login interface where
 you don't need to re-connect to the server*/
void logout(int * socket_fd){
    message_t msg;
    char buffer[maximum_buffer_size];
    // leave session if in a session
    if (session_status == ONLINE){
        leave_session(socket_fd);
    }
    // set flags and prepare and send EXIT message
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

/* join a session if user not in one
 prevent user from joining multiple sessions*/
void join_session(int * socket_fd, char * session_id){
    // IF user in a session, print error message and return
    if (session_status == ONLINE){
        fprintf(stderr, "You are already in a session\n");
        return;
    }
    // prepare and send JOIN message
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

/* leave session if user in a session*/
void leave_session(int * socket_fd){
    // if user not in a session, print error message and return
    if (session_status == OFFLINE){
        fprintf(stderr, "You are not in a session!\n");
        return;
    }
    // prepare and send LEAVE_SESS message
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

/* create a session if user not in one*/
void create_session(int * socket_fd, char * session_id){
    // if user in a session, print error message and return
    if (session_status == ONLINE){
        fprintf(stderr, "You are already in a session\n");
        return;
    }

    // prepare and send a request to create a session
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

/* send a request to the server to list all
 online users and their sessions*/
void list(int * socket_fd){
    // prepare and send a request to list all online users
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

/* send a request to the server to list all
 users in a session specified by session_id*/
void user_list(int * socket_fd, char * session_id){
    message_t msg;
    char buffer[maximum_buffer_size];
    // if request to list users in a forbidden session,
    // print error message and return
    if(strcmp(session_id, NOT_IN_SESSION) == 0){
        fprintf(stdout,"Session Query Not permitted\n \"not in session\" is not a session");
        return;
    }
    // prepare and send a request to list all users in a session
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

/* terminate the program by setting flags
 and calling necessary functions before
 exiting the program*/
void terminate_program(int * socket_fd){
    if (server_connection_status == ON){
        logout(socket_fd);
    }
    termination_signal = ON;
}

/* send a request to the server to promote a user
 * by the username*/
void promote(int *socket_fd, char* username){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    // prepare and send a request to promote a user
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

/* send a request to the server to kick a user
 * by the username in the session where the
 * current user is an admin */
void kick(int *socket_fd, char* username){
    if (!length_validate((int)strlen(username), MAX_NAME)){
        fprintf(stderr, "Username needs to be under length of %d\n", MAX_NAME);
        return;
    }
    // prepare and send a request to kick a user
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

/* set the server connection status*/
void set_server_connection_status(int status){
    server_connection_status = status;
}

/* validate the length of some variable*/
int length_validate(int received_length, int expected_length){
    if (received_length >= expected_length){
        printf("Parameter too long\n");
        return validation_failure;
    }
    return validation_success;
}

/* validate the number of parameters*/
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

/* check if the command is a valid command
 * and return corresponding code*/
int check_commands(char* command){
    if (command[0] != '/') return message_command;
    if (strcmp (command, "/login") == 0) return login_command;
    if (strcmp (command, "/logout") == 0) return logout_command;
    if (strcmp (command, "/quit") == 0) return quit_command;
    if (strcmp (command, "/joinsession") == 0) return join_session_command;
    if (strcmp (command, "/leavesession") == 0) return leave_session_command;
    if (strcmp (command, "/createsession") == 0) return create_session_command;
    if (strcmp (command, "/list") == 0) return list_command;
    if (strcmp (command, "/create") == 0) return create_command;
    if (strcmp (command, "/kick") == 0) return kick_command;
    if (strcmp (command, "/promote") == 0) return promote_command;
    if (strcmp (command, "/userlist") == 0) return userlist_command;
    return invalid_command;

}
