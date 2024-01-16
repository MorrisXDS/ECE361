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
int main(){
    printf("Hello World!\n");
    
    return 0;
}
