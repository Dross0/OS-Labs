//
// Created by Андрей Гайдамака on 09.12.2020.
//

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define TIMEOUT_S 1
#define BUFFER_SIZE 1024

#define SERVER_PORT 2050
int sockFd = 0;


int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Invalid argument number=%d, expected=%d, params=%s (ip)", argc, 2, argv[0]);
        return 1;
    }
    char recvBuff[BUFFER_SIZE] = {0};
    struct sockaddr_in serverAddress;

    if((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, argv[1], &serverAddress.sin_addr) <= 0){
        printf("Cant parse address=%s\n", argv[1]);
        return 2;
    }

    if(connect(sockFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        printf("Connect failed\n");
        return 3;
    }

    fd_set sfds;
    fd_set inds;

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_S;
    timeout.tv_usec = 0;
    int readBytes = 0;

    while(1) {
        FD_ZERO(&sfds);
		FD_SET(sockFd, &sfds);

		FD_ZERO(&inds);
		FD_SET(STDIN_FILENO, &inds);

		int select_socket = select(sockFd + 1, &sfds, NULL, NULL, &timeout);
		if(errno != 0) {
			close(sockFd);
			break;
		}
		if(select_socket) {
			if ((readBytes = read(sockFd, recvBuff, sizeof(recvBuff) - 1)) > 0){
				recvBuff[readBytes] = 0;
				if(fputs(recvBuff, stdout) == EOF){
					printf("Puts error\n");
				}
			}
		}

		if(select(STDIN_FILENO + 1, &inds, NULL, NULL, &timeout)) {
			if ((readBytes = read(STDIN_FILENO, recvBuff, BUFFER_SIZE - 1)) > 0){
				recvBuff[readBytes - 1] = 0;
                write(sockFd, recvBuff, strlen(recvBuff));
				if(strcmp("/exit", recvBuff) == 0) {
					close(sockFd);
					break;
				}
			}
		}
	}
	if(readBytes < 0){
		printf("Read error\n");
	}
	return 0;
}
