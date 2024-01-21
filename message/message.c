#include "message.h"

void message_to_buffer(message_t* msg,  unsigned char* buffer){
    sprintf((char *)buffer, "%d:%d:%s:", msg->type, msg->size, msg->source);
    unsigned long size = strlen((char *)buffer);
    for (unsigned int index = 0; index < msg->size; ++index) {
        buffer[size+index] =  msg->data[index];
    }
};

void buffer_to_message(message_t* msg, const unsigned char* buffer){
    unsigned char content[12];
    int count = 0;
    int index = 0;
    while(buffer[count] != ':'){
        content[index] = buffer[count] ;
        count++;
        index++;
    }
    content[index] = '\0';
    msg->type = atoi((char *)content);
    count++;

    index = 0;
    while(buffer[count] != ':'){
        content[index] = buffer[count] ;
        count++;
        index++;
    }
    content[index] = '\0';
    msg->size = atoi((char *)content);
    count++;

    index = 0;
    while(buffer[count] != ':'){
        msg->source[index] = buffer[count] ;
        count++;
        index++;
    }
    msg->source[index] = '\0';
    count++;

    for (int i = 0; i < msg->size; ++i) {
        msg->data[i] = buffer[count+i];
    }
};

int error_check(int condition, int check, const char *error_message){
    if (condition != check) return 1;
    perror(error_message);
    return 0;
};

void decode_server_response(unsigned int type, char* response){
    switch (type) {
        case LO_ACK:
            printf("You are logged in!\n");
            break;
        case LO_NAK:
            printf("Authentication failed. %s\n", response);
            break;
        case JN_ACK:
            printf("Session Starts!\n");
            break;
        case JN_NAK:
            printf("Session not created, %s\n", response);
            break;
        case QU_ACK:
            printf("List:\n %s\n", response);
            break;
        default:
            printf("invalid response\n");
            break;
    }
};

