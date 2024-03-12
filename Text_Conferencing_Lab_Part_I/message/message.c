#include "message.h"


/* fill a message in the form of message_t
  message_t {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};*/
void fill_message(message_t* message, unsigned int type, unsigned int size, char* source, char* data) {
    // set message size and type
    message->size = size;
    message->type = type;
    // if size is 0, zero out the message data
    if (size == 0) {
        memset(message->data, 0, sizeof(message->data));
    }
    // if not a message type, copy the data directly
    else if (message->type != MESSAGE) {
        strcpy((char*)message->data, data);
    }
    // if message type, handle the data by
    // skipping null characters and assign
    // char by char and add a null character
    // at the end
    else {
        for (int i = 0; i < message->size; ++i) {
            message->data[i] = data[i];
            if (data[i] == '\0') {
                message->data[i] = ' ';
                continue;
            }
        }
        message->data[message->size - 1] = '\0';
    }
    // specify who the message was originally from
    strcpy((char*)message->source, source);
}

/* convert message to an agreed string buffer format
 * <message_type>:<message_size>:<message_source>:data
 * the length of data is determined by message_size as
 * there might be multiple null terminators in message
 * data which are regarded as the end of string.
 * incomplete stringification may occur if using
 * string operations*/
int message_to_string(message_t* message, unsigned int message_data_size, char* buffer) {
    sprintf(buffer, "%d:%d:%s:", message->type, message_data_size, message->source);
    unsigned int count = strlen(buffer);
    if (message_data_size == 0) {
        memset(message->data, 0, sizeof(message->data));
    }
    else {
        unsigned long start_index = strlen(buffer);
        strcat(buffer, (char*)message->data);
        for (int i = 0; i < message_data_size; ++i) {
            buffer[start_index + i] = (char)message->data[i];
            count++;
        }
        buffer[count] = '\0';
    }
    return (int)count;
}

/* convert string buffer to message
 * String functions are not used due to
 * the fact that there may be multiple
 * null terminators in the data which
 * are considered the end of strings*/
void string_to_message(message_t* message, const char* buffer) {
    unsigned char content[12];
    int count = 0;
    int index = 0;
    while (buffer[count] != ':') {
        content[index] = buffer[count];
        count++;
        index++;
    }
    content[index] = '\0';
    message->type = atoi((char*)content);
    count++;

    index = 0;
    while (buffer[count] != ':') {
        content[index] = buffer[count];
        count++;
        index++;
    }
    content[index] = '\0';
    message->size = atoi((char*)content);
    count++;

    index = 0;
    while (buffer[count] != ':') {
        message->source[index] = buffer[count];
        count++;
        index++;
    }
    message->source[index] = '\0';
    count++;

    for (int i = 0; i < message->size; ++i) {
        message->data[i] = buffer[count + i];
    }

}

/* send message to socket
 * return SEND_ERROR if send is not fully successful
 * return CONNECTION_REFUSED if connection is refused
 * return bytes sent if successful*/
int send_message(const int* socket_fd, unsigned int message_size, char* message) {
    size_t bytes_sent = send(*socket_fd, message, message_size, 0);
    if (bytes_sent == -1 || bytes_sent != message_size) {
        return SEND_ERROR;
    }
    else if (bytes_sent == 0) {
        return CONNECTION_REFUSED;
    }

    return (int)bytes_sent;
}

/* receive message from socket
 * return RECEIVE_ERROR if receive errors (network issues)
 * return CONNECTION_REFUSED if connection is refused
 * return bytes received if successful*/
int receive_message(const int* socket_fd, char* message) {
    size_t bytes_read = recv(*socket_fd, message, maximum_buffer_size, 0);
    if (bytes_read == -1) {
        return RECEIVE_ERROR;
    }
    else if (bytes_read == 0) {
        return CONNECTION_REFUSED;
    }
    return (int)bytes_read;
}

void fill_and_send_message(const int* socket_fd, unsigned int type, char* source, char* data) {
    message_t reply_message;
    char buffer[maximum_buffer_size];
    memset(buffer, 0, sizeof(buffer));
    memset(&reply_message, 0, sizeof(reply_message));
    int size = 0;
    if (data != NULL) size = strlen(data);
    fill_message(&reply_message, type, size, source, data);
    message_to_string(&reply_message, reply_message.size, buffer);
    send_message(socket_fd, strlen(buffer), buffer);
}