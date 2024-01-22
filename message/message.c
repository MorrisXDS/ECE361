#include "message.h"


void fill_message(message_t* msg, unsigned int type, unsigned int size,char* source, char* data){
    memset(msg, 0, sizeof(message_t));
    msg->type = type;
    msg->size = size;
    strcpy((char *)msg->source, (char *)source);
    if (size == 0) {
        memset(msg->data, 0, MAX_DATA);
        return;
    }
    for (unsigned int index = 0; index < size; ++index) {
        msg->data[index] = data[index];
    }
};

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
        case NS_ACK:
            printf("You just created a session\n");
            break;
        case JN_ACK:
            printf("Session Starts!\n");
            break;
        case JN_NAK:
            printf("Session not created, %s\n", response);
            break;
        case QU_ACK:
            printf("%s", response);
            break;
        default:
            puts(response);
            break;
    }
};

