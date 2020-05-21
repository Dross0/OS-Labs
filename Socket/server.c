#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>


#define BUFFER_SIZE 1024

int main(int argc, char ** argv){
    char ADDRESS[] = "mysocket";
    int sockfd = 0;
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return 1;
    }    
    struct sockaddr_un sockAddress;
    sockAddress.sun_family = AF_UNIX;
    int addressLen = strlen(ADDRESS);
    strncpy(sockAddress.sun_path, ADDRESS, addressLen);
    unlink(ADDRESS);
    if (bind(sockfd, (struct sockaddr*) &sockAddress, sizeof(sockAddress)) < 0){
        perror("bind");
        return 2;
    }
    if (listen(sockfd, 3) < 0){
        perror("listen");
        return 3;
    }
    int clientSock = 0;
    struct sockaddr clientAddr;
    socklen_t clen = 0;
    if ((clientSock = accept(sockfd, (struct sockaddr *)&clientAddr, &clen)) < 0){
        perror("accept");
        return 4;
    }
    char buffer[BUFFER_SIZE] = {0};
    int strSize = 0;
    while ((strSize = read(clientSock, buffer, BUFFER_SIZE))){
        buffer[strSize] = 0;
        printf("%s\n", buffer);
    }
    close(clientSock);
    return 0;
}