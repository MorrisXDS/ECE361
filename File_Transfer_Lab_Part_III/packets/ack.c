#include "ack.h"

void fill_the_ack(struct ack_packet* ptr, unsigned int tf,
    unsigned int fn, char recv){
        ptr->total_fragment = tf;
        ptr->fragment_no = fn;
        ptr->received = recv;
};

void ack_to_buffer(struct ack_packet* ptr, char* data){
    sprintf(data, "%d:%d:%d:", ptr->total_fragment, ptr->fragment_no, 
        ptr->received);
};

void buffer_to_ack(struct ack_packet* ptr, char* data){
    char content[12];
    int count = 0;
    int index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    ptr->total_fragment = (unsigned int)atoi(content);
    count++;

    index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    ptr->fragment_no = (unsigned int)atoi(content);
    count++;

    index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    ptr->received = (unsigned int)atoi(content);
};
