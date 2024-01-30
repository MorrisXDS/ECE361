#include "../packets/packet.h"

#define max_len 256
#define null_padded_max_len max_len-1


int capacity = 10;
int total_frag = 0;

// This deliver is based on the assumption that the local address 
// will always be IPv4
int main(int argc, char* argv[]){
	if(argc != 3){
		printf("the number of parameters is inccorrect!!\n");
		exit(3);
	}

    printf("Hello, Please transfer the file in the following format\nftp filename: ");
    
	char ftp[3];
	char file_name[256];

	scanf("%s %s", ftp, file_name);

	// check if the format is correct
	int input_buffer;

	input_buffer = getchar();

    if (input_buffer != 10 && input_buffer != EOF){
		printf("Too many arguments! Please re-start and try again\n");
		exit(1);
	}

	if(strcmp(ftp,"ftp") != 0) {
		printf("Incorrect Protocol! Please re-start and try again\n");
		exit(1);
	}

	int fd;
    if((fd = open(file_name,O_RDONLY)) == -1){
        printf("The file \"%s\" does not exist!\n", file_name);
        exit(errno);
    }

    struct packet* packets;

    int index = 0;
    packets = malloc(sizeof (struct packet) * capacity);
    if(packets == NULL){
        perror("allocation failed please try again later!\n");
    }

    char buffer[file_frag_size];

    // read file content
    ssize_t bytes_read = 0;
    while((bytes_read = read(fd, buffer, file_frag_size)) > 0){
        write_file_name(&(packets[index]),file_name);
        write_frag_no(&(packets[index]), index+1);
        write_frag_size(&(packets[index]),(unsigned int)bytes_read);
        write_frag_data(&(packets[index]),buffer, (int)bytes_read);

        printf("packet #%d is ready!\n", index+1);

        index++;

        if(index == capacity){
            capacity *= 2;
            packets = realloc(packets, sizeof(struct packet) * capacity);
            if(packets == NULL){
                perror("allocation failed please limit your file size and try again!\n");
            }
        }
    }

    close(fd);

    total_frag = index;

    printf("all %d packets are ready!\n", total_frag);

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

    ssize_t  number_bytes;
    char recipient_buffer[max_len];
    socklen_t from_size;
    from_size = sizeof (servinfo->ai_addr);

    for (int i = 0; i < total_frag; i++){
        packets[i].total_frag = total_frag;
        char data[heap_size];
        printf("packet #%d is being sent out\n", i+1);
        convert_to_string(&packets[i], data, file_name);
        number_bytes = sendto(socketfd, data, heap_size, 0,
                                 (struct sockaddr *)servinfo->ai_addr, sizeof(struct sockaddr));


        number_bytes = recvfrom(socketfd, recipient_buffer, null_padded_max_len,
                                0, (struct sockaddr *)servinfo->ai_addr, &from_size);

        if(number_bytes == -1)
            break;

        recipient_buffer[number_bytes] = '\n';

        printf("the server replied: %s \n", recipient_buffer);

        if (strcmp(recipient_buffer,"packet missing!") == 0) break;
        
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
    free(packets);
    close(socketfd);
    freeaddrinfo(servinfo);

	printf("\"%s\" was sent successfully!\n", file_name);

    return 0;
}
