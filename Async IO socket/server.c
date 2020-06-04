#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <aio.h>

#define BUFFER_SIZE 1024
#define TIMEOUT 10000
#define MAX_CLIENTS 32

struct aiocb * readfds[MAX_CLIENTS] = {0};
int clientsAmount = 0;

void add_client(struct aiocb * clients[], int clientSocket, char * buffer, size_t buffer_size){
    struct aiocb * readrq = (struct aiocb *)calloc(1, sizeof(struct aiocb));
    readrq->aio_fildes = clientSocket;
    readrq->aio_buf = buffer;
    readrq->aio_nbytes = buffer_size;
    readrq->aio_lio_opcode = LIO_READ;
    readrq->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    readrq->aio_sigevent.sigev_signo = SIGIO;
    readrq->aio_sigevent.sigev_value.sival_ptr = readrq;
    

    int firstEmpty = 0;
    for (; clients[firstEmpty]; ++firstEmpty);
    printf("Client %d connected\n", firstEmpty);
    if (firstEmpty == MAX_CLIENTS){
        printf("Max of clients\n");
        return;
    }
    clients[firstEmpty] = readrq;
    clientsAmount++;
}

void deleteClient(struct aiocb * request){
    int i = 0;
    for (i = 0; i < MAX_CLIENTS; ++i){
        if (readfds[i] == request){
            readfds[i] = NULL;
            int j = 0;
            for (j = i + 1; j < MAX_CLIENTS; ++j){
                readfds[j - 1] = readfds[j];
            }
            readfds[MAX_CLIENTS - 1] = NULL;
            clientsAmount--;
            free(request);
            return;
        }
    }
}

void apply_requests(struct aiocb * clients[]){
    int i = 0;
    while (clients[i]){
        aio_read(clients[i]);
        i++;
    }
}

void handler(int signo, siginfo_t * siginfo, void * code){
    struct aiocb * request = (struct aiocb *)siginfo->si_value.sival_ptr;
    int len = aio_return(request);
    char * buf = 0;
    
    if (len > 0){
        buf = (char *) request->aio_buf;
        buf[len] = 0;
        write(1, buf, len);
        write(1, "\n", 1);
    }
    else if (len == -1){
        perror("aio_return");
        exit(1);
    }
    int cliSock = request->aio_fildes;
    int nbytes = request->aio_nbytes;
    if (len == 0){
        deleteClient(request);
    }
}

int main(int argc, char ** argv){
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

    char buffer[BUFFER_SIZE] = {0};
    int clientSock = 0;
    struct sockaddr clientAddr;
    socklen_t clen = 0;   
     

    struct sigaction act;
    act.sa_flags = SA_SIGINFO| SA_RESTART;
    act.sa_sigaction = handler;
    sigaction(SIGIO, &act, NULL);   
    while (1){
         if ((clientSock = accept(serverSocket, (struct sockaddr *)&clientAddr, &clen)) >= 0){
            add_client(readfds, clientSock, buffer, BUFFER_SIZE);
        }
         apply_requests(readfds);
    }
    return 0;
}
