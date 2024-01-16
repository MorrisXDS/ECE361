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


// This deliver is based on the assumption that the local address 
// will always be IPv4
int main(int argc, char* argv[]){
	if(argc != 3){
		printf("the number of parameters is inccorrect!!\n");
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

    return 0;
}
