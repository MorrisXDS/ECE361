#ifndef CLIENT_H
#define CLIENT_H
#include "../message/message.h"

#define command_number 7
#define maximum_buffer_size 1200
#define maximum_parameter_size 5

#define OFF 0
#define ON 1



void login(int * socket_fd, char * ip_address, char * port, char * username, char * password, pthread_t* thread);
void logout(int * socket_fd);
void join_session(int * socket_fd, char * session_id);
void leave_session(int * socket_fd);
void create_session(int * socket_fd, char * session_id);
void list(int * socket_fd);
void terminate_program(int * socket_fd);
void* server_message_handler(void* socket_fd);
void set_connection_status(int status);

#endif