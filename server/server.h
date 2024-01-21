#ifndef SERVER_H
#define SERVER_H
#include "../message/message.h"


//TYPES
#define SUCCESS_LOGIN 2

//Functional
#define CONNECTION_CAPACITY 10
#define LIST_CAPACITY 10
#define MAX_PASSWORD_LENGTH 20
#define THREAD_CAPACITY 10

//global variables
char login_error_types[2][20] = {"Username not found", "Incorrect password"};

//structs
struct user {
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_PASSWORD_LENGTH];
};

int set_user_list();
void* connection_handler(void * accept_fd);
int verify_login(unsigned char * username, unsigned char * password);
void generate_login_response(int login_status, message_t* msg,
        unsigned char* buffer ,int buffer_size);

#endif