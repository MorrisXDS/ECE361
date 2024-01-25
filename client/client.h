#ifndef CLIENT_H
#define CLIENT_H
#include "../message/message.h"

#define maximum_buffer_size 1200
#define login_parameter_size 5
#define logout_parameter_size 1
#define join_session_parameter_size 2
#define leave_session_parameter_size 1
#define create_session_parameter_size 2
#define list_parameter_size 1
#define quit_parameter_size 1
#define create_parameter_size 5

#define validation_failure 0
#define validation_success 1

#define thread_number 10

#define OFF 0
#define ON 1



void login(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread);
void user_registration(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread);
void logout(int * socket_fd);
void join_session(int * socket_fd, char * session_id);
void leave_session(int * socket_fd);
void create_session(int * socket_fd, char * session_id);
void list(int * socket_fd);
void terminate_program(int * socket_fd);
void* server_message_handler(void* socket_fd);
void set_server_connection_status(int status);
int length_validate(int received_length, int expected_length);
int parameter_count_validate(int received_count, int expected_count);

#endif