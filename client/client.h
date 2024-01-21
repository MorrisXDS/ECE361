#ifndef CLIENT_H
#define CLIENT_H

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


#endif