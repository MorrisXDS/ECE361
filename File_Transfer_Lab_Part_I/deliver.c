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

// This deliver is based on the assumption that the local address 
// will always be IPv4
int main(int argc, char* argv[]){
	if(argc != 3){
		fprintf(stdout, "usage: deliver <hostname> <port>\n");
		exit(3);
	}

    printf("Hello, Please transfer the file in the following format\n ftp <file name>: ");
    
	char ftp[3];
	char file_name[256];

	scanf("%s %s", ftp, file_name);

	// check if the format is correct
	int input_buffer;

	input_buffer = getchar();

    if (input_buffer != 10 && input_buffer != EOF){
		printf("1:Incorrect format. Please restart the client host!\n");
		exit(1);
	}

	if(strcmp(ftp,"ftp") != 0) {
		printf("2:Incorrect format. Please restart the client host!\n");
		exit(1);
	}

	// assume reading in regular forms (non-binary)
	FILE* transfer_file;
	if((transfer_file = fopen(file_name,"r") ) == NULL){
		printf("The file \"%s\" does not exist!\n", file_name);
		exit(errno);
	}
	
	// close the file. we're not going to use it in part 1 anyways
	fclose(transfer_file);

	// cited from page 38 "Beej's Guide to Network Programming", modified
    struct addrinfo hints, *servinfo, *p; int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
        return 1;
    }
    // end of citation

	int socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socketfd == -1){
        printf("Error: failed to create a select socket!\n");
        exit(errno);
    }


	int number_bytes = sendto(socketfd, ftp, sizeof(ftp), 0, (struct sockaddr *)servinfo->ai_addr, sizeof(struct sockaddr));
    if (number_bytes == -1){
        printf("Error: failed to send!\n");
        exit(errno);
    }

	printf("\"%s\" is sent successfully!\n");

	socklen_t to_size;
    to_size = sizeof (servinfo->ai_addr);

	char recipient_buffer[max_len];

	number_bytes = recvfrom(socketfd, recipient_buffer, null_padded_max_len, 0, (struct sockaddr *)servinfo->ai_addr, &to_size);
    if (number_bytes == -1){
        printf("Error: failed to receive!\n");
        exit(errno);
    }

	printf("recvied \"%s\"\n", recipient_buffer);
	
	if(strcmp(recipient_buffer, "yes") != 0) {
        printf("recevied a message other than \"yey\". exit now\n");
        exit(1);
    }

	printf("A file transfer can start.\n");

    return 0;
}
