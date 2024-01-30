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
    if(argc != 2){
		fprintf(stdout, "usage: server <port>\n");
		exit(3);
	}

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
    int number_bytes = recvfrom(socketfd,recipient_buffer, null_padded_max_len, 0, &from_addr, &from_size);
    
    if (number_bytes == -1){
        printf("Error: failed to receive!\n");
        exit(errno);
    }

    // endi the buffer with null terminator
    recipient_buffer[number_bytes] = '\0';

    // reply to the client based on whats come in
    if(strcmp(recipient_buffer, "ftp") == 0){
        number_bytes = sendto(socketfd,"yes", sizeof("yes"), 0, &from_addr, from_size);
        if (number_bytes == -1){
            printf("Error: failed to send!\n");
            exit(errno);
        }
        printf("server replied with yes!\n");
    }
    else{
        number_bytes = sendto(socketfd,"no", sizeof("no"), 0, &from_addr, from_size);
        if (number_bytes == -1){
            printf("Error: failed to send!\n");
            exit(errno);
        }
        printf("server replied with no!\n");
    }
    
    return 0;
}
