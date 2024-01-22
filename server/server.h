#ifndef SERVER_H
#define SERVER_H
#include "../message/message.h"

//TYPES
#define SUCCESS_LOGIN 3
#define USERNAME_ERROR 0
#define PASSWORD_ERROR 1

//USER STATUS
#define OFFLINE 0
#define ONLINE 1
#define ALREADY_LOGIN 2
#define NOT_IN_SESSION "Not in session"

//Functional
#define CONNECTION_CAPACITY 10
#define LIST_CAPACITY 10
#define MAX_PASSWORD_LENGTH 20
#define THREAD_CAPACITY 10
#define MAX_SESSION_NAME 20
#define MAX_PORT_LENGTH 6

#define user_t struct user


//global variables
char login_error_types[3][45] = {"Username not found", "Incorrect password", "You were already logged in somewhere else"};

struct user {
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_PASSWORD_LENGTH];
    unsigned char status;
    char session_id[MAX_SESSION_NAME];
    char ip_address[INET_ADDRSTRLEN];
    char port[MAX_PORT_LENGTH];
    int socket_fd;
};



int set_user_list();
int return_user_index(char * username);
void* connection_handler(void * accept_fd);
int verify_login(unsigned char * username, unsigned char * password);
void generate_login_response(int login_status, char * username, message_t* msg,
        unsigned char* buffer ,int buffer_size);
void get_active_user_list(message_t * msg);
void send_message_in_a_session(message_t * msg, char* session_id);

//void send_a_message(int *fd, message_t * msg);

#endif