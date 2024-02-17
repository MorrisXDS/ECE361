#ifndef ACK_H
#define ACK_H
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

#define copied 1
#define lost 0

struct ack_packet{
    unsigned int total_fragment;
    unsigned int fragment_no;
    char received;
};

void fill_the_ack(struct ack_packet* ptr, unsigned int tf,
    unsigned int fn, char recv);
void ack_to_buffer(struct ack_packet* ptr, char* data);
void buffer_to_ack(struct ack_packet* ptr, char* data);

#endif