#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define max_len 50
#define null_padded_max_len max_len-1

// This server is based on the assumption that the local address 
// will always be IPv4
int main(int argc, char* argv[]){
    // cited from page 38 "Beej's Guide to Network Programming", modified
    struct addrinfo hints, *servinfo, *p; int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
        return 1;
    }
    // end of citation

    int socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socketfd == -1){
        printf("Error: failed to create a select socket!\n");
        exit(errno);
    } 

    int check = bind(socketfd, (struct sockaddr *)(servinfo->ai_addr), (socklen_t) (servinfo->ai_addrlen));
    if (check == -1){
        printf("Error: failed to bind a select socket!\n");
        exit(errno);
    }

    char recipient_buffer[max_len];

    struct sockaddr from_addr;
    socklen_t from_size;

    from_size = sizeof (from_addr);

    printf("server starts receiving ...\n");
    check = recvfrom(socketfd,recipient_buffer, null_padded_max_len, 0, &from_addr, &from_size);
    
    if (check == -1){
        printf("Error: failed to receive!\n");
        exit(errno);
    }

    
    return 0;
}
