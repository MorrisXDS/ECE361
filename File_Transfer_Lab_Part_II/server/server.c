#include "../packets/packet.h"

unsigned int total_frag = 0;
#define name_legnth 256

// This server is based on the assumption that the local address
// will always be IPv4
int main(int argc, char* argv[]){
    if (argc != 2){
        printf("the number of parameters is wrong. Please check it!\n");
        exit(1);
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

    struct sockaddr from_addr;
    socklen_t from_size;
    from_size = sizeof (from_addr);

    struct packet receiver = {0};

    printf("server starts receiving ...\n");

    char buffer[heap_size];

    ssize_t number_bytes = recvfrom(socketfd, buffer, heap_size, 0, &from_addr, &from_size);

    if (number_bytes == -1) {
        perror("Failed to receive!\n");
        exit(errno);
    }

    char name[name_legnth];

    buffer_to_packet(&receiver, buffer, name);

    printf("filename is %s\n", receiver.filename);

    int fd = open(receiver.filename, O_CREAT | O_RDWR, S_IRWXU);

    if (fd == -1){
        perror("Failed to open the file");
        perror(receiver.filename);
        perror("\n");
    }

    ssize_t bytes_written;
    total_frag = receiver.total_frag;

    char messages[256];

    sprintf(messages, "Recevied %d packets out of %d, waiting for %d more", 1, total_frag, total_frag-1);

    number_bytes = sendto(socketfd, messages, strlen(messages)+1, 0, &from_addr, from_size);
    if(number_bytes == -1){
        perror("failed to reply!");
        exit(errno);
    }

    

    bytes_written = write(fd, receiver.filedata, receiver.size);
    if(bytes_written == -1){
        perror("write failed!");
        exit(errno);
    }

    for(int i = 1; i < total_frag; i++){
        number_bytes = recvfrom(socketfd, buffer, sizeof(buffer), 0, &from_addr, &from_size);
        if(number_bytes == -1){
            perror("receipt failed!");
            exit(errno);
        }
        
        char message[256];

        sprintf(message, "Recevied %d packets out of %d, waiting for %d more", i+1, total_frag, total_frag-i-1);

        number_bytes = sendto(socketfd, message, strlen(message)+1, 0, &from_addr, from_size);
        if(number_bytes == -1){
            perror("failed to reply!");
            exit(errno);
        }

        buffer_to_packet(&receiver, buffer, name);

        bytes_written = write(fd, receiver.filedata, receiver.size);
        if(bytes_written == -1){
            perror("write failed!");
            exit(errno);
        }
    }

    char see_you[256] = "End of File Transfer. Thank you!\n";

    number_bytes = sendto(socketfd, see_you, 
        strlen(see_you)+1, 0, &from_addr, from_size);
    if(number_bytes == -1){
        perror("failed to reply!");
        exit(errno);
    }

    close(fd);
    close(socketfd);
    freeaddrinfo(servinfo);

    return 0;
}
