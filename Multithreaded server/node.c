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
void user_disconnect(int clients[], int clientIndex){
    printf("client %d disconnected\n", clientIndex);
    close(clients[clientIndex]);
    clients[clientIndex] = -1;
}


int main(int argc, char *argv[]) {
    if (argc != 2){
        return 1;
    }

    int clients[CLIENTS_AMOUNT] = {-1};
    memset(clients, -1, sizeof(int) * CLIENTS_AMOUNT);

    char sendBuff[1025] = {0};
    
    int listenfd  = socket(AF_INET, SOCK_STREAM, 0);
    shutdown(listenfd, 2);
    
    
    int listenPort = atoi(argv[1]);
    if (listenPort >= 65535){
        printf("Port must be less than 65535");
        return 2;
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(listenPort);

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        return 1;
    }

    listen(listenfd, 5);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set lfds;
    fd_set cfds;
    fd_set readfds;
    
    char c[BUFFER_SIZE + 1];
    int read_bytes;
    
    while(1) {

        FD_ZERO(&lfds);
        FD_SET(listenfd, &lfds);

        if(select(listenfd + 1, &lfds, NULL, NULL, &timeout)){
            
            int newClientIndex = find_free_user_index(clients);
            
            if(newClientIndex == -1){
                continue;
            }
            
            clients[newClientIndex] = accept(listenfd, (struct sockaddr*)NULL, NULL);
            
            printf("client %d accepted\n", clients[newClientIndex]);
        }
            
        for(int j = 0; j < CLIENTS_AMOUNT; j++) {
                
            if(clients[j] == -1){
                continue;
            }
            
            FD_ZERO(&cfds);
            FD_SET(clients[j], &cfds);
            
            if(select(clients[j]+1, &cfds, NULL, NULL, &timeout)) {
                if((read_bytes = read(clients[j], &c, BUFFER_SIZE)) <= 0) {
                   user_disconnect(clients, j);
                   continue;
                }
                c[read_bytes] = '\0';
                printf("client %d: %s\n", j, c);
            }
                
        }
            
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        if(select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout)) {
            if(read_bytes = read(STDIN_FILENO, &c, BUFFER_SIZE)) {
                c[read_bytes] = '\0';
                
                for(int j = 0; j < CLIENTS_AMOUNT; j++) {
                    if(clients[j] == -1){
                        continue;
                    }
                    write(clients[j], c, read_bytes);
                }
            }
        }
    }
}


