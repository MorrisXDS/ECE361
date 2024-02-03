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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define file_frag_size 1000
#define sending_buffer_size 2000
#define NOT_SET 0

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

void write_packets(struct packet* file_frag, unsigned int total_frag, unsigned int frag_number,
        unsigned int frag_size, char* file_name, const char* data);
int convert_to_string(struct packet* file_frag, char * data, char* filename);
void buffer_to_packet(struct packet* file_frag, char * data, char * name);

#endif