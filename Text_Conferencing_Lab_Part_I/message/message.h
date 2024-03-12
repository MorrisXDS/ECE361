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

//user roles
#define USER 0
#define ADMIN 1

//access status
#define OFFLINE 0
#define ONLINE 1

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
#define CREATE 13
#define RG_ACK 14
#define RG_NAK 15
#define KICK 16
#define KI_ACK 17
#define KI_NAK 18
#define KI_MESSAGE 19
#define PROMOTE 20
#define PM_ACK 21
#define PM_NAK 22
#define PM_MESSAGE 23
#define SESS_QUERY 24
#define SQ_ACK 25
#define SQ_NAK 26


#define NOT_IN_SESSION "Not in session"

//Define Errors
#define RECEIVE_ERROR (-1)
#define SEND_ERROR (-1)
#define CONNECTION_REFUSED (0)


//Struct Macros
#define MAX_NAME 17
#define MAX_PASSWORD_LENGTH 20
#define MAX_SESSION_LENGTH 20
#define MAX_DATA 1000
#define ACC_BUFFER_SIZE 1500

#define maximum_buffer_size 1200

typedef struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} message_t;

void fill_message(message_t* message, unsigned int type,
    unsigned int size, char* source, char* data);
int message_to_string(message_t* message, unsigned int message_data_size, char* buffer);
void string_to_message(message_t* message, const char* buffer);
int send_message(const int* socket_fd, unsigned int message_size, char* message);
int receive_message(const int* socket_fd, char* message);
void fill_and_send_message(const int* socket_fd, unsigned int type, char* source, char* data);

#endif