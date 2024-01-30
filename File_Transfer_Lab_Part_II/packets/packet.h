#ifndef DEFINITIONS_H
#define DEFINITIONS_H
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

#define file_frag_size 1000
#define heap_size 2000

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

void write_file_name(struct packet* file_frag, char* file_name);
void write_frag_no(struct packet* file_frag, unsigned int frag_number);
void write_frag_size(struct packet* file_frag, unsigned int frag_size);
void write_frag_total_frag(struct packet* file_frag, unsigned int frag_number);
void write_frag_data(struct packet* file_frag, const char* buffer, int byte_write);

char* get_file_name(struct packet* file_frag);
unsigned int get_frag_no(struct packet* file_frag);
unsigned int get_frag_size(struct packet* file_frag);
unsigned int get_frag_total_frag(struct packet* file_frag);
void convert_to_string(struct packet* file_frag, char * data, char* filename);
void buffer_to_packet(struct packet* file_frag, char * data, char * name);

#endif