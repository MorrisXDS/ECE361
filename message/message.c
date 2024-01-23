#include "message.h"


void fill_message(message_t * message, unsigned int type,unsigned int size, char * source, char * data){
    message->size = size;
    message->type = type;
    if (size == 0){
        memset(message->data, 0, sizeof(message->data));
    }
    else{
        strcpy((char*)message->data, data);
    }
    strcpy((char*)message->source, source);
}

void message_to_string(message_t * message, unsigned int message_data_size, char * buffer){
    sprintf(buffer, "%d:%d:%s:", message->type, message_data_size, message->source);
    if (message_data_size == 0){
        memset(message->data, 0, sizeof(message->data));
    }
    else{
        strncat(buffer, (char*)message->data, message_data_size);
    }
}
void string_to_message(message_t * message, char * data){
    char * token = strtok(data, ":\0");
    message->type = atoi(token);
    token = strtok(NULL, ":\0");
    message->size = atoi(token);
    token = strtok(NULL, ":\0");
    strcpy((char*)message->source, token);
    token = strtok(NULL, ":\0");
    if (token == NULL){
        memset(message->data, 0, sizeof(message->data));
        return;
    }
    strcpy((char*)message->data, token);
}
int send_message(const int * socket_fd, unsigned int message_size, char * message){
    size_t bytes_sent = send(*socket_fd, message, message_size, 0);
    if(bytes_sent == -1 || bytes_sent != message_size){
        return SEND_ERROR;
    }
    else if(bytes_sent == 0){
        return CONNECTION_REFUSED;
    }

    return (int)bytes_sent;
}

int receive_message(const int * socket_fd, char * message){
    size_t bytes_read = recv(*socket_fd, message, maximum_buffer_size, 0);
    if(bytes_read == -1){
        return RECEIVE_ERROR;
    }
    else if(bytes_read == 0){
        return CONNECTION_REFUSED;
    }
    return (int)bytes_read;
}

