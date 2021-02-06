//
// Created by Андрей Гайдамака on 09.12.2020.
//

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
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <ctype.h>


#define MAX_CLIENTS_AMOUNT 10
#define DEBUG 1
#define ADDRESS_BUF_SIZE 256
#define CACHE_SIZE 10
#define BUFFER_SIZE 1024
#define EMPTY -1

#define SERVER_PORT 2050

#define PORT_LEN 5

int socketConnect(char *host, in_port_t port){

    if (DEBUG) printf("getting host...\n");

    printf("host in function = %s\n", host);

    struct hostent *hp = gethostbyname(host);
    if(hp == NULL){
        perror("gethostbyname");
        return -1;
    }

    if (DEBUG) printf("host got!\n");
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    if (DEBUG) printf("opening host...\n");

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1){
        perror("socket");
        return -1;
    }
    int on = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if (DEBUG) printf("host openned!\n");

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        return -1;
    }

    return sock;
}

typedef struct cache {
    int page_size;
    char* title;
    char* page;
} cache_t;

typedef struct url {
    char * host;
    char * path;
    int port;
} url_t;

void addr_init(struct sockaddr_in *addr, int port) {
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);
}

int findFreeClientIndex(const int *clients) {
    int freeClientIndex = 0;
    while (freeClientIndex < MAX_CLIENTS_AMOUNT && clients[freeClientIndex] >= 0){
        freeClientIndex++;
    }
    return freeClientIndex;
}

void acceptNewClient(int listenfd, int *clients) {
    int freeClientIndex = findFreeClientIndex(clients);
    clients[freeClientIndex] = accept(listenfd, (struct sockaddr*)NULL, NULL);
    time_t ticks = time(NULL);
    char sendBuff[BUFFER_SIZE + 1];
    snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
    write(clients[freeClientIndex], sendBuff, strlen(sendBuff));
}

int tryAcceptNewClient(
        int listenfd,
        int *clients,
        int alreadyConnectedClientsNumber,
        struct timeval * timeout,
                fd_set *lfds) {
    if(alreadyConnectedClientsNumber < MAX_CLIENTS_AMOUNT) {
        if(select(4, lfds, NULL, NULL, timeout)){
            if(DEBUG)printf("[DEBUG]: Can read from listen\n");
            acceptNewClient(listenfd, clients);
            alreadyConnectedClientsNumber++;
        }
    }
    return alreadyConnectedClientsNumber;
}

void handleClientDisconnecting(
        int *clients,
        int *clientsHttpSockets,
        int *cacheToClient,
        int *sentBytes,
        int clientIndex) {
    printf("client %d disconnecting...\n", clientIndex);
    close(clients[clientIndex]);
    close(clientsHttpSockets[clientIndex]);
    clients[clientIndex] = EMPTY;
    clientsHttpSockets[clientIndex] = EMPTY;
    cacheToClient[clientIndex] = EMPTY;
    sentBytes[clientIndex] = 0;
}

void freeURL(url_t *pUrl) {
    free(pUrl->path);
    free(pUrl->host);
    free(pUrl);
}

url_t * parseURL(char *urlBuffer) {
    url_t * url = (url_t *) malloc(sizeof(url_t));
    url->path = NULL;
    url->host = NULL;
    url->port = 80;
    size_t urlBufferSize = strlen(urlBuffer);
    int startPortIndex = 0;
    for (size_t strIndex = 0; strIndex < urlBufferSize; strIndex++){
        if (urlBuffer[strIndex] == ':'){
            if (startPortIndex){
                freeURL(url);
                return NULL;
            }
            startPortIndex = strIndex;
            char port[PORT_LEN + 1] = {0};
            size_t portStrIndex = strIndex + 1;
            int portIndex = 0;
            while (portIndex <  PORT_LEN && portStrIndex < urlBufferSize && isdigit(urlBuffer[portStrIndex])){
                port[portIndex++] = urlBuffer[portStrIndex];
                portStrIndex++;
            }
            url->port = atoi(port);
        }
        if (urlBuffer[strIndex] == '/') {
            if (strIndex + 1 == urlBufferSize) {
                urlBuffer[strIndex] = '\0';
                break;
            }
            url->host = (char *) malloc(sizeof(char) * (startPortIndex + 1));
            url->path = (char *) malloc(sizeof(char) * (urlBufferSize - strIndex));
            strncpy(url->host, urlBuffer, startPortIndex  ? startPortIndex : strIndex);
            strncpy(url->path, &(urlBuffer[strIndex + 1]), urlBufferSize - strIndex - 1);
            printf("host = %s\n", url->host);
            printf("path = %s\n", url->path);
            break;
        }
    }
    return url;
}



void readToCache(
        cache_t *cache,
        int currentCacheSize,
        const int *clientsHttpSockets,
        int clientIndex) {

    int offset = 0;
    int read_bytes = 0;
    while((read_bytes = read(clientsHttpSockets[clientIndex], &((cache[currentCacheSize].page)[offset]), BUFFER_SIZE)) != 0) {
        offset += read_bytes;
        if(DEBUG)printf("[DEBUG]: cache[%d].page size = %d\n", currentCacheSize , cache[currentCacheSize].page_size);
        if(DEBUG)printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);
        cache[currentCacheSize].page_size = offset;
        cache[currentCacheSize].page = (char*)realloc(cache[currentCacheSize].page, offset + BUFFER_SIZE + 1);
    }
}

int tryFindAtCache(
        const cache_t *cache,
        int cacheEntitiesAmount,
        int *cacheToClient,
        int *sentBytes,
        int clientIndex,
        const char *urlBuffer) {

    for(int i = 0; i < cacheEntitiesAmount; i++) {
        if(strcmp(cache[i].title, urlBuffer) == 0) {
            cacheToClient[clientIndex] = i;
            sentBytes[clientIndex] = 0;
            printf("Page found in cache!\n");
            return 1;
        }
    }
    return 0;
}

char *createRequest(const url_t *url) {
    char* request = (char*)malloc(sizeof(char) * (ADDRESS_BUF_SIZE + 16));
    strcpy(request, "GET /");
    strcat(request, url->path);
    strcat(request, "\r\n");
    return request;
}

void sendRequest(
        const int *clientsHttpSockets,
        int clientIndex,
        const url_t *url) {
    if(url->path == NULL){
        write(clientsHttpSockets[clientIndex], "GET /\r\n", strlen("GET /\r\n"));
    } else{
        char *request = createRequest(url);
        if(DEBUG)printf("[DEBUG]: REQUEST: %s", request);
        write(clientsHttpSockets[clientIndex], request, strlen(request));
        free(request);
    }
}

int resendFromCache(
        const cache_t *cache,
        int connectedClientsAmount,
        int *clients,
        int *cacheToClient,
        int *sent_bytes,
        int clientIndex) {
    int written_bytes = 0;
    if(cacheToClient[clientIndex] != EMPTY) {
        if (sent_bytes[clientIndex] < cache[cacheToClient[clientIndex]].page_size) { 
            int bytes_left = cache[cacheToClient[clientIndex]].page_size - sent_bytes[clientIndex];
            if ((written_bytes = write(clients[clientIndex], &(cache[cacheToClient[clientIndex]].page[sent_bytes[clientIndex]]), BUFFER_SIZE < bytes_left ? BUFFER_SIZE : bytes_left)) != 0) {
                sent_bytes[clientIndex] += written_bytes;
            } else if (written_bytes == EMPTY) {
                if(DEBUG)printf("[DEBUG]: Client disconnected!\n");
                cacheToClient[clientIndex] = EMPTY;
                clients[clientIndex] = EMPTY;
                connectedClientsAmount--;
            }
        }
    }
    return connectedClientsAmount;
}

int main(int argc, char *argv[]) {

    int listenfd = 0;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    cache_t * cache = (cache_t*)malloc(sizeof(cache_t) * CACHE_SIZE);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int clients[MAX_CLIENTS_AMOUNT];                   
    int clientsHttpSockets[MAX_CLIENTS_AMOUNT];        
    int cacheToClient[MAX_CLIENTS_AMOUNT];             
    int sentBytes[MAX_CLIENTS_AMOUNT];            


    for(int k = 0; k < MAX_CLIENTS_AMOUNT; k++) {
        clients[k] = EMPTY;
        clientsHttpSockets[k] = EMPTY;
        cacheToClient[k] = EMPTY;
        sentBytes[k] = EMPTY;
    }

    addr_init(&serv_addr, SERVER_PORT);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, MAX_CLIENTS_AMOUNT);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set lfds;
    fd_set cfds;

    if(DEBUG)printf("[DEBUG]: listenfd = %d\n", listenfd);

    int connectedClientsNumber = 0;
    int cacheSize = 0;
    while(1) {

        FD_ZERO(&lfds);
        FD_SET(listenfd, &lfds);

        connectedClientsNumber = tryAcceptNewClient(listenfd, clients, connectedClientsNumber, &timeout, &lfds);
        

        for(int clientIndex = 0; clientIndex < MAX_CLIENTS_AMOUNT; clientIndex++) {
            
            if(clients[clientIndex] < 2) continue;

            FD_ZERO(&cfds);
            FD_SET(clients[clientIndex], &cfds);

            if(clientsHttpSockets[clientIndex] == EMPTY && select(clients[clientIndex] + 1, &cfds, NULL, NULL, &timeout)) {  //may ||
                char urlBuffer[ADDRESS_BUF_SIZE];
                int read_bytes = read(clients[clientIndex], &urlBuffer, ADDRESS_BUF_SIZE);
                if(read_bytes) {
                    urlBuffer[read_bytes] = '\0';
                    printf("\"%s\"\n", urlBuffer);
                    if(strcmp(urlBuffer, "/exit") == 0) {
                        connectedClientsNumber--;
                        handleClientDisconnecting(clients, clientsHttpSockets, cacheToClient, sentBytes, clientIndex);
                        continue;
                    }
                    url_t * url = parseURL(urlBuffer);
                    if (url == NULL){
                        if (DEBUG) printf("[DEBUG]: URL parsing fail\n");
                        continue;
                    }
                    if(DEBUG)printf("[DEBUG]: Connecting socket...\n");
                    printf("PORT=%d\n", url->port);
                    printf("clientIndex = %d, MAX_CLIENTS_AMOUNT = %d\n", clientIndex, MAX_CLIENTS_AMOUNT);

                    int findAtCache = tryFindAtCache(cache, cacheSize, cacheToClient, sentBytes, clientIndex, urlBuffer);

                    if (!findAtCache) {
                        clientsHttpSockets[clientIndex] = socketConnect(url->host, url->port);

                        if(clientsHttpSockets[clientIndex] == EMPTY) {
                            write(clients[clientIndex], "Failed connect!\n", strlen("Failed connect!\n"));
                            continue;
                        }

                        if(DEBUG)printf("[DEBUG]: Socket connected.\n");
                        
                        sendRequest(clientsHttpSockets, clientIndex, url);


                        cache[cacheSize].title = (char*)malloc(sizeof(char) * strlen(urlBuffer));
                        strcpy(cache[cacheSize].title, urlBuffer);

                        cache[cacheSize].page_size = BUFFER_SIZE;
                        cache[cacheSize].page = (char*)malloc(BUFFER_SIZE + 1);

                        cacheToClient[clientIndex] = cacheSize;
                        sentBytes[clientIndex] = 0;


                        if(DEBUG)printf("[DEBUG]: reading...\n");

                        readToCache(cache, cacheSize, clientsHttpSockets, clientIndex);

                        close(clientsHttpSockets[clientIndex]);

                        cache[cacheSize].page[cache[cacheSize].page_size + 1] = '\0';
                        cacheSize++;
                        if(DEBUG)printf("[DEBUG]: cache read!\n");
                    }

                    freeURL(url);
                }
            }
            connectedClientsNumber = resendFromCache(cache, connectedClientsNumber, clients, cacheToClient, sentBytes, clientIndex);
        }
    }
}

