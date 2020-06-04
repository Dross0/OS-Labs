#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define BUFFER_SIZE 1024


int main(int argc, char ** argv){
    char ADDRESS[] = "mysocket";
    int sockfd = 0;
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        perror("client socket");
        return 1;
    }
    struct sockaddr_un sockAddress;
    sockAddress.sun_family = AF_UNIX;
    int addressLen = strlen(ADDRESS);
    strncpy(sockAddress.sun_path, ADDRESS, addressLen + 1);
    if (connect(sockfd, (struct sockaddr*) &sockAddress, sizeof(sockAddress)) < 0){
        perror("connect");
        return 2;
    }
    char buffer[BUFFER_SIZE] = {0};
    while (strcmp(buffer, "quit")){
        int strSize = 0;
        char * tmp;
        if (!fgets(buffer, BUFFER_SIZE, stdin)){
            break;
        }
        int size = strlen(buffer);
        buffer[--size] = 0;
        write(sockfd, buffer, size);
    }
    close(sockfd);
    return 0;
}
