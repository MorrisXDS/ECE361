#ifndef MESSAGE_H
#define MESSAGE_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//DATA TYPES
#define LOGIN 0
#define LO_ACK 1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10
#define QUERY 11
#define QU_ACK 12

//Define Errors
#define FD_ERROR (-1)
#define BIND_ERROR (-1)
#define LISTEN_ERROR (-1)
#define CONNECT_ERROR (-1)
#define ACCEPT_ERROR (-1)
#define RECEIVE_ERROR (-1)
#define SEND_ERROR (-1)
#define SETUP_ERROR (-1)
#define LOGIN_ERROR (-1)
#define GETLINE_ERROR (-1)
#define USERNAME_ERROR 0
#define PASSWORD_ERROR 1
#define COMMAND_NOT_FOUND -1

//Struct Macros
#define MAX_NAME 17
#define MAX_DATA 1000
#define ACTUAL_NAME_LENGTH 16
#define ACC_BUFFER_SIZE 1500

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

void message_to_buffer(struct message* msg, unsigned char* buffer);
void buffer_to_message(struct message* msg, const unsigned char* buffer);
int error_check(int condition, int check, const char *error_message);
void decode_server_response(unsigned int type, char* response);

#endif