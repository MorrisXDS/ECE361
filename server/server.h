#ifndef SERVER_H
#define SERVER_H
#include "session.h"

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

//Functional
#define CONNECTION_CAPACITY 10
#define LIST_CAPACITY 10

#define THREAD_CAPACITY 10

#define MAX_PASSWORD 17
#define BACKLOG 10
#define database_path "database.txt"

#define user_t struct user

unsigned int USER_LIST_CAPACITY = 16;


int set_up_database();
int verify_login(unsigned char * username, unsigned char * password);
char* select_login_message(int index);
void user_log_in(char * username, char * password, unsigned char status, int *socket_fd);
void server_side_user_exit(char* username);
void server_side_session_join(char * username, char * session_id);
void server_side_session_leave(char * username);
void server_side_session_create(char * username, char * session_id);
int session_name_check(char * session_id);
void print_active_list(user_list_t * list);
void get_active_list(user_list_t * list, char * buffer);
void* connection_handler(void * accept_fd);
char* session_response_message(int value);
void send_message_in_a_session(message_t * msg, char * session_id);


#endif