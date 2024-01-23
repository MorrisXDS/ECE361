#ifndef SERVER_H
#define SERVER_H
#include "../message/message.h"

//TYPES
#define SUCCESS_LOGIN 3
#define USERNAME_ERROR 0
#define PASSWORD_ERROR 1
#define FILE_ERROR (-1)
#define LIST_EMPTY (-1)


//USER STATUS
#define OFFLINE 0
#define ONLINE 1
#define ALREADY_LOGIN 2
#define NOT_IN_SESSION "Not in session"
#define IP_NOT_FOUND "ip not found"
#define ADMIN 1
#define USER 0

//Functional
#define CONNECTION_CAPACITY 10
#define LIST_CAPACITY 10
#define MAX_PASSWORD_LENGTH 20
#define THREAD_CAPACITY 10
#define MAX_SESSION_COUNT 20
#define MAX_PASSWORD 17
#define BACKLOG 10
#define database_path "database.txt"

#define user_t struct user

unsigned int USER_LIST_CAPACITY = 16;
unsigned int USER_COUNT = 0;


//global variables
char login_error_types[3][45] = {"Username not found", "Incorrect password", "You were already logged in somewhere else"};

struct user {
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_PASSWORD_LENGTH];
    unsigned char status;
    char session_id[MAX_SESSION_COUNT];
    char ip_address[INET6_ADDRSTRLEN]; //compatibility for both IPv4 and IPv6
    unsigned int port;
    int socket_fd;
    char role;
};



int user_list_init();
user_t* return_user_by_id(int id);
int expand_user_list();
char* get_user_session_id(int id);
int user_id_bound_check(int id);
user_t* return_user_by_session_id(char * session_id);
user_t* return_user_by_username(unsigned char * username);
int verify_login(unsigned char * username, unsigned char * password);
char* select_login_message(int index);
void user_log_in(char * username, char * password, unsigned char status, int *socket_fd);
void add_user(char * username, char * password, unsigned char status, int socket_fd, char * ip_address, unsigned int port);
void server_side_user_exit(int id);
void set_user_session_id(int id, char * session_id);
void server_side_session_leave(int id);
unsigned int get_user_count();
unsigned int increment_user_count();
unsigned int decrement_user_count();
int return_user_index(char * username);
void* connection_handler(void * accept_fd);
void generate_login_response(int login_status, char * username, message_t* msg,
        unsigned char* buffer ,int buffer_size);


#endif