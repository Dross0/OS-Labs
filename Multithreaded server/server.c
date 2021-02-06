#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

#define CLIENTS_AMOUNT 510
#define BUFFER_SIZE 80


int find_free_user_index(int clients[]) {
    for(int i = 0; i < CLIENTS_AMOUNT; i++){
        if(clients[i] == -1) {
            return i;
        }
    }
    return -1;
}
        
int main(int argc, char *argv[]) {
    if (argc != 4){
        return 1;
    }
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char sendBuff[1025] = {0};

    int clients[CLIENTS_AMOUNT];
    int clients_trans[CLIENTS_AMOUNT];
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(clients, -1, CLIENTS_AMOUNT * sizeof(int));
    memset(clients_trans, -1, CLIENTS_AMOUNT * sizeof(int));
    
   int listenPort = atoi(argv[3]);
    if (listenPort >= 65535){
        printf("Port must be less than 65535");
        return 2;
    }

    int translatedPort = atoi(argv[2]);
    if (translatedPort >= 65535){
        printf("Port must be less than 65535");
        return 2;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(listenPort);

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        return 1;
    }

    listen(listenfd, CLIENTS_AMOUNT);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set lfds;
    fd_set cfds;

    while(1) {

        FD_ZERO(&lfds);
        FD_SET(listenfd, &lfds);

        if(select(listenfd + 1, &lfds, NULL, NULL, &timeout)){
            
            int newClientIndex = find_free_user_index(clients);
            printf("Client try connect\n");
            if(newClientIndex == -1){
                continue;
            }
            
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            client_addr.sin_family = AF_INET;

            if(inet_pton(AF_INET, argv[1], &client_addr.sin_addr)<=0){
                printf("\n inet_pton error\n");
                return 1;
            }

            clients[newClientIndex] = accept(listenfd, (struct sockaddr*)NULL, NULL);
            
            do{
                close(clients_trans[newClientIndex]);
                clients_trans[newClientIndex] = socket(AF_INET, SOCK_STREAM, 0);
                client_addr.sin_port = htons(translatedPort);
            } while (connect(clients_trans[newClientIndex], (struct sockaddr *)&client_addr, sizeof(client_addr)) != 0);
            
            printf("translate %d to %d\n", newClientIndex, clients_trans[newClientIndex]);
        }
        
        int readBytes;
        char c[BUFFER_SIZE+1];
        
        for(int j = 0; j < CLIENTS_AMOUNT; j++) {
            if(clients[j] == -1){
                continue;
            }
            FD_ZERO(&cfds);
            FD_SET(clients[j], &cfds);
            if(select(clients[j]+1, &cfds, NULL, NULL, &timeout)) {
                if((readBytes = read(clients[j], &c, BUFFER_SIZE)) <= 0) {
                    printf("client %d disconnected\n", j);
                    close(clients_trans[j]);
                    close(clients[j]);
                    clients_trans[j] = -1;
                    clients[j] = -1;
                    continue;
                } 
                c[readBytes] = '\0';
                write(clients_trans[j], c, readBytes);
                printf("client %d: %s\n", j, c);
            }
        }
        
        for(int j = 0; j < CLIENTS_AMOUNT; j++) {
            if(clients_trans[j] == -1){
                continue;
            }
            FD_ZERO(&cfds);
            FD_SET(clients_trans[j], &cfds);
            if(select(clients_trans[j]+1, &cfds, NULL, NULL, &timeout)) {
                if(readBytes = read(clients_trans[j], &c, BUFFER_SIZE)) {
                    c[readBytes] = '\0';
                    write(clients[j], c, readBytes);
                    printf("client %d: %s\n", j, c);
                }
            }
        }
    }
}

