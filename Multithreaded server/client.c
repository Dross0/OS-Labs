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
int sockFd = 0;

void create_function(int server_port,struct sockaddr_in *serverAddress, char * ip){
    if((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Error : Could not create socket \n");
        exit(-1);
    }

    memset(serverAddress, 0, sizeof(*serverAddress));

    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(server_port);

    if(inet_pton(AF_INET, ip, &serverAddress->sin_addr) <= 0){
        printf("Cant parse address=%s\n", ip);
        exit(-2);
    }

    if(connect(sockFd, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) < 0){
        printf("Connect failed\n");
        exit(-3);
    }
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Invalid number of arguments"); 
        return 1;
    }
    int server_port = atoi(argv[2]);

    struct sockaddr_in serverAddress;
    create_function(server_port, &serverAddress, argv[1]);

    fd_set sfds;
    fd_set inds;

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_S;
    timeout.tv_usec = 0;

    int readBytes = 0;
    char recvBuff[BUFFER_SIZE] = {0};

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
                write(sockFd, recvBuff, readBytes);
				if(strcmp("/EXIT", recvBuff) == 0) {
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
