#include "../packets/packet.h"
#include "../packets/ack.h"
#include <time.h>

unsigned int total_frag = 0;
#define name_length 256

// This server is based on the assumption that the local address
// will always be IPv4
int main(int argc, char* argv[]){
    if (argc != 2){
        printf("usage: server <port number>\n");
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
    srand(time(NULL));
    
    while(1) {
        char buffer[sending_buffer_size] = {0};
        ssize_t number_bytes = recvfrom(socketfd, buffer, sending_buffer_size, 0, &from_addr, &from_size);
        if (number_bytes == -1) {
            perror("Failed to receive!\n");
            exit(errno);
        }
        char messages[256];
        int rand_num = 0;
        struct ack_packet response_packet = {0};
        char name[name_length] = {0};
        printf("server starts receiving ...\n");
        struct packet receiver = {0};
        rand_num = (rand() % 100) +1;

        if(rand_num > 5){
            fill_the_ack(&response_packet, total_frag, 1, recevd);
            ack_to_buffer(&response_packet, messages);
            number_bytes = sendto(socketfd, messages, strlen(messages) + 1, 0, &from_addr, from_size);
            if (number_bytes == -1) {
                perror("failed to reply!");
                exit(errno);
            }
        }
        else{
            
            continue;
        }

        if (strcmp(buffer, "done\n") == 0) {
            printf("user finished file transfer. Now quitting\n");
            break;
        }

        buffer_to_packet(&receiver, buffer, name);

        int fd = open(receiver.filename, O_CREAT | O_RDWR, S_IRWXU);

        if (fd == -1) {
            perror("Failed to open the file");
            perror(receiver.filename);
            perror("\n");
        }

        ssize_t bytes_written;
        total_frag = receiver.total_frag;

        bytes_written = write(fd, receiver.filedata, receiver.size);
        if (bytes_written == -1) {
            perror("write failed!");
            exit(errno);
        }
        
        for (int i = 1; i < total_frag; i++) {
            memset(buffer, 0 , sizeof(buffer));
            number_bytes = recvfrom(socketfd, buffer, sizeof(buffer), 0, &from_addr, &from_size);
            if (number_bytes == -1) {
                perror("receipt failed!");
                exit(errno);
            }


            rand_num = (rand() % 100) +1;
            if(rand_num > 5){
                memset(&response_packet, 0, sizeof(struct ack_packet));
                memset(messages, 0, sizeof(messages));
                fill_the_ack(&response_packet, total_frag, i+1, recevd);
                ack_to_buffer(&response_packet, messages);
                number_bytes = sendto(socketfd, messages, strlen(messages) + 1, 0, &from_addr, from_size);
                if (number_bytes == -1) {
                    perror("failed to reply!");
                    exit(errno);
                }
            }
            else{
                continue;
            }

            buffer_to_packet(&receiver, buffer, name);
            bytes_written = write(fd, receiver.filedata, receiver.size);
            if (bytes_written == -1) {
                perror("write failed!");
                exit(errno);
            }
        }

        char end_of_file[256];
        sprintf(end_of_file, "End of File Transfer for %s.\n", name);

        number_bytes = sendto(socketfd, end_of_file,
                              strlen(end_of_file) + 1, 0, &from_addr, from_size);

        if (number_bytes == -1) {
            perror("failed to reply!");
            exit(errno);
        }
        close(fd);
    }

    close(socketfd);
    freeaddrinfo(servinfo);
    return 0;
}
