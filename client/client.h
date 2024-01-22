#ifndef CLIENT_H
#define CLIENT_H
#include "../message/message.h"

#define terminal_buffer_size 128
#define command_number 7
#define message_id 8

//global variables
char commands[command_number][20] = {"/login",
                        "/logout",
                        "/joinsession",
                        "/leavesession",
                        "/createsession",
                        "/list",
                        "/quit"};

int check_command(char * command);
void connect_to_server(char * ip_address, char * port, int * socket_fd);
void take_terminal_input(char ** terminal_buffer);
void* response_handler(void* arg);
void command_to_type(char * command, unsigned int* type);

#endif