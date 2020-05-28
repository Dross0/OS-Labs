#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>


#define POLL_SIZE 32
#define BUFFER_SIZE 1024
#define TIMEOUT 10000

int main(int argc, char ** argv){
    struct pollfd poll_set[POLL_SIZE];

    char ADDRESS[] = "mysocket";
    int serverSocket = 0;
    if ((serverSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return 1;
    }

    struct sockaddr_un sockAddress;
    sockAddress.sun_family = AF_UNIX;
    int addressLen = strlen(ADDRESS);
    strcpy(sockAddress.sun_path, ADDRESS);
    unlink(ADDRESS);
    if (bind(serverSocket, (struct sockaddr*) &sockAddress, sizeof(sockAddress)) < 0){
        perror("bind");
        return 2;
    }

    
    if (listen(serverSocket, 3) < 0){
        perror("listen");
        return 3;
    }

    memset(poll_set, 0, sizeof(poll_set));
    poll_set[0].fd = serverSocket;
    poll_set[0].events = POLLIN;
    int numfds = 1;

    char buffer[BUFFER_SIZE] = {0};

    while (numfds){
        if (poll(poll_set, numfds, TIMEOUT) == -1){
            perror("poll");
            return 2;
        }
        for (int fd_index = 0; fd_index < numfds; ++fd_index){
            if (poll_set[fd_index].revents & POLLIN){
                if (poll_set[fd_index].fd == serverSocket){
                    int clientSock = 0;
                    struct sockaddr clientAddr;
                    socklen_t clen = 0;
                    if ((clientSock = accept(serverSocket, (struct sockaddr *)&clientAddr, &clen)) < 0){
                        perror("accept");
                        return 4;
                    }
                    poll_set[numfds].fd = clientSock;
                    poll_set[numfds].events = POLLIN;
                    numfds++;
                    printf("Client %d connected\n", clientSock);
                }
                else{
                    int nread = 0;
                    ioctl(poll_set[fd_index].fd, FIONREAD, &nread);
                    if (nread == 0){
                        close(poll_set[fd_index].fd);
                        --numfds;
                        printf("Client %d removed\n", poll_set[fd_index].fd);
                        poll_set[fd_index].events = 0;
                        poll_set[fd_index].fd = -1;

                    }
                    else{
                        int strSize = 0;
                        if ((strSize = read(poll_set[fd_index].fd, buffer, BUFFER_SIZE)) == 0 ){
                            perror("read");
                            return 5;
                        }
                        buffer[strSize] = 0;
                        printf("%s\n", buffer);
                    }
                }
            }
        }
    }
    return 0;
}