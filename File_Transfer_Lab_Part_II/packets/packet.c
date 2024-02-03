//
// Created by MorrisSun on 2024-01-18.
//
#include "packet.h"

void write_packets(struct packet* file_frag, unsigned int total_frag, unsigned int frag_number,
                   unsigned int frag_size, char* file_name, const char* data){
    memset(file_frag, 0, sizeof(struct packet));
    file_frag->total_frag = total_frag;
    file_frag->frag_no = frag_number;
    file_frag->size = frag_size;
    file_frag->filename = file_name;
    memcpy(file_frag->filedata, data, file_frag->size);
}

int convert_to_string(struct packet* file_frag, char * data, char* filename){
    sprintf(data, "%d:%d:%d:%s:", file_frag->total_frag, file_frag->frag_no,
        file_frag->size, filename);

    unsigned int bound = strlen(data) + file_frag->size;
    unsigned int start_point = strlen(data);
    for(unsigned int i = start_point; i < bound; i++){
        data[i] = file_frag->filedata[i-start_point];
    }
    return (int)bound;
}

void buffer_to_packet(struct packet* file_frag, char * data, char * name){
    char content[12];
    int count = 0;
    int index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    file_frag->total_frag = (unsigned int)atoi(content);
    count++;

    index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    file_frag->frag_no = (unsigned int)atoi(content);
    count++;

    index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    file_frag->size = (unsigned int)atoi(content);
    count++;

    index = 0;
    while(data[count] != ':'){
        name[index] = data[count] ;
        count++;
        index++;
    }
    name[index] = '\0';
    file_frag->filename = name;
    count++;

    for (int i = 0; i < file_frag->size; ++i) {
        file_frag->filedata[i] = data[count+i];
    }
}