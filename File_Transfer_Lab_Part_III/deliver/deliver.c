#include "../packets/packet.h"
#include "../packets/ack.h"
#include <time.h>

#define max_len 256
#define null_padded_max_len (max_len-1)
#define second_to_microsecond 1000000

double time_interval = 0;

double new_estimate_RTT(double sample_RTT, double old_estimated_RTT){
    double alpha = 0.125;
    return (1-alpha)*old_estimated_RTT + alpha*sample_RTT;
}

double new_dev_RTT( double dev_RTT, double sample_RTT, double estimated_RTT){
    double beta = 0.25;
    return (1-beta)*dev_RTT + beta* (double)fabs(sample_RTT - estimated_RTT);
}

// This deliver is based on the assumption that the local address
// will always be IPv4
int main(int argc, char* argv[]){
    if(argc != 3){
        printf("usage: deliver <hostname> <host port>\n");
        exit(3);
    }

    struct addrinfo hints, *servinfo;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(errno);
    }
    // end of citation

    int socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socketfd == -1){
        printf("Error: failed to create a select socket!\n");
        exit(errno);
    }

    while(1){
        printf("Hello, Please transfer the file in the following format\nftp filename: ");
        char ftp[4];
        char file_name[256];
        char terminal_buffer[1024];
        double estimated_RTT = 0;
        double dev_RTT = 0;

        char * temp = fgets(terminal_buffer, 1023, stdin);
        if (temp == NULL){
            perror("failed to read the input!\n");
            memset(terminal_buffer, 0, 1024);
            continue;
        }

        printf("terminal buffer is %s\n", terminal_buffer);
        if (strcmp(terminal_buffer, "done\n") == 0){
            printf("Bye for now!\n");
            sendto(socketfd, terminal_buffer, strlen(terminal_buffer), 0,
                   (struct sockaddr *)servinfo->ai_addr, sizeof(struct sockaddr));
            break;
        }

        char* token = strtok(terminal_buffer, " \n");
        strcpy(ftp, token);
        token = strtok(NULL, " \n");
        if (token == NULL){
            printf("file name missing!\n");
            continue;
        }
        strcpy(file_name, token);
        if (strtok(NULL, " \n") != NULL){
            printf("too many arguments!\n");
            continue;
        }

        if(strcmp(ftp,"ftp") != 0) {
            printf("Incorrect Protocol! Please re-start and try again\n");
            continue;
        }

        int fd;
        if((fd = open(file_name,O_RDONLY)) == -1){
            printf("The file \"%s\" does not exist!\n", file_name);
            continue;
        }

        struct stat file_stats;
        int status = stat(file_name, &file_stats);
        if (status == -1){
            perror("failed to get the file stats!\n");
            continue;
        }

        ssize_t size = file_stats.st_size;
        int total_frag = ceil((double) size/file_frag_size);

        ssize_t  number_bytes;
        char recipient_buffer[max_len];
        socklen_t from_size;
        from_size = sizeof (*servinfo->ai_addr);

        struct packet sender;
        char buffer[file_frag_size];
        char data[sending_buffer_size];

        for (int i = 0; i < total_frag; ++i) {
            struct timeval timeout = {0};
            memset(buffer, 0, file_frag_size);
            memset(&sender, 0, sizeof(struct packet));
            ssize_t bytes_read = read(fd, buffer, file_frag_size);
            if (bytes_read == -1){
                perror("failed to read the file!\n");
                exit(errno);
            }
            if (bytes_read == 0){
                fprintf(stdout, "finished reading the file!\n");
                break;
            }
            write_packets(&sender, total_frag, i+1,
                          bytes_read, file_name, buffer);
            memset(data, 0, sending_buffer_size);


            if (i == 0){
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
            }
            else{
                timeout.tv_sec = (int)time_interval;
                timeout.tv_usec = (int)((time_interval - (int)time_interval)* second_to_microsecond);
            }

            if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
                perror("failed to set the timeout!\n");
                exit(errno);
            }

            printf("packet #%d is being sent out\n", i+1);
            int data_size = convert_to_string(&sender, data, file_name);
            number_bytes = sendto(socketfd, data, data_size, 0,
                                  (struct sockaddr *)servinfo->ai_addr, sizeof(struct sockaddr));
            clock_t start_time = clock();
            

            if(number_bytes == -1)
                break;

            number_bytes = recvfrom(socketfd, recipient_buffer, null_padded_max_len,
                                    0, (struct sockaddr *)servinfo->ai_addr, &from_size);
            clock_t end_time = clock();
            if(number_bytes == -1){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    fprintf(stderr,"no data received from the server assume packet %d has been lost!\n", i+1);
                    fprintf(stderr,"resending packet %d\n", i+1);
                    i--;
                    continue;
                }
                else {
                    perror("failed to receive!\n");
                    exit(errno);
                }
            }


            double RTT = ((double)end_time - (double)start_time)/CLOCKS_PER_SEC;
            if(i == 0 ){
                estimated_RTT = RTT;
                //assume the dev_RTT is 1/2 of the RTT
                dev_RTT = RTT/2;
            }
            else{
                estimated_RTT = new_estimate_RTT(RTT, estimated_RTT);
                dev_RTT = new_dev_RTT(dev_RTT,RTT, estimated_RTT);
            }
            //time_out period formula
            time_interval = estimated_RTT + 4*dev_RTT;

            printf("Round Trip Time for %d: %f\n", i+1, RTT);
            recipient_buffer[number_bytes] = '\n';
            printf("the server replied: %s \n", recipient_buffer);
        }

        if (number_bytes == -1){
            printf("Error: failed!\n");
            exit(errno);
        }

        char* goodbye_msg = malloc(sizeof(char)*max_len);
        number_bytes = recvfrom(socketfd, goodbye_msg, null_padded_max_len,
                                0, (struct sockaddr *)servinfo->ai_addr, &from_size);

        if(number_bytes == -1){
            perror("failed to receive!\n");
            exit(0);
        }

        printf("GoodBye Message from the Server: %s",goodbye_msg);

        free(goodbye_msg);
        printf("\"%s\" was sent successfully!\n", file_name);
    }
    close(socketfd);
    freeaddrinfo(servinfo);

    return  0;
}
