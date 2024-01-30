//
// Created by MorrisSun on 2024-01-18.
//
#include "packet.h"

#ifndef null_terminator_compensation
#define null_terminator_compensation 1
#endif

void write_file_name(struct packet* file_frag, char* file_name){
    file_frag->filename = file_name;
};

void write_frag_no(struct packet* file_frag, unsigned int frag_number){
    file_frag->frag_no = frag_number;
};

void write_frag_size(struct packet* file_frag, unsigned int frag_size){
    file_frag->size = frag_size;
};

void write_frag_total_frag(struct packet* file_frag, unsigned int frag_number){
    file_frag->total_frag = frag_number;
};

void write_frag_data(struct packet* file_frag, const char* buffer, int byte_write){
    memcpy(file_frag->filedata, buffer, byte_write);
};

char* get_file_name(struct packet* file_frag){
    return file_frag->filename;
};

unsigned int get_frag_no(struct packet* file_frag){
    return file_frag->frag_no;
};

unsigned int get_frag_size(struct packet* file_frag){
    return file_frag->size;
};

unsigned int get_frag_total_frag(struct packet* file_frag){
    return file_frag->total_frag;
};

void convert_to_string(struct packet* file_frag, char * data, char* filename){
    sprintf(data, "%d:%d:%d:%s:", file_frag->total_frag, file_frag->frag_no, 
        file_frag->size, filename);
    
    int bound = strlen(data) + file_frag->size;
    int start_point = strlen(data);
    for(int i = start_point; i < bound; i++){
        data[i] = file_frag->filedata[i-start_point];
    }
};

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
    write_frag_total_frag(file_frag, (unsigned int)atoi(content));
    count++;

    index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    write_frag_no(file_frag, (unsigned int)atoi(content));
    count++;

    index = 0;
    while(data[count] != ':'){
        content[index] = data[count] ;
        count++;
        index++;
    }
    content[index] = '\n';
    write_frag_size(file_frag, (unsigned int)atoi(content));
    count++;

    index = 0;
    while(data[count] != ':'){
        name[index] = data[count] ;
        count++;
        index++;
    }
    write_file_name(file_frag, name);
    count++;

    for (int i = 0; i < file_frag->size; ++i) {
        file_frag->filedata[i] = data[count+i];
    }
};