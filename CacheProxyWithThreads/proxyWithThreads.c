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
#include <pthread.h>
#include <ctype.h>

#define MAX_CLIENTS_AMOUNT 10
#define DEBUG 1
#define ADDRESS_BUF_SIZE 256
#define CACHE_SIZE 3
#define BUFFER_SIZE 1024
#define PORT_LEN 5
#define PORT 2050

typedef struct cache {
	int page_size;
	char* title;
	char* page;
	pthread_mutex_t mutex;
	long lastTime;
} cache_t;

pthread_t* clientThreads;
cache_t * cache;

int cacheFreeIndex = 0;

pthread_mutex_t cacheIndexMutex;
pthread_mutex_t reallocOperationMutex;

typedef struct url {
    char * host;
    char * path;
    int port;
} url_t;

typedef struct threadArg{
    int clientFD;
    int clientIndex;
} threadArg_t;

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


int socketConnect(char *host, in_port_t port){
    printf("host in function = %s\n", host);

    struct hostent *hp = gethostbyname(host);
    if(hp == NULL){
        perror("gethostbyname");
        return -1;
    }
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1){
        perror("socket");
        return -1;
    }
    int on = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        return -1;
    }

    return sock;
}

void addrInit(struct sockaddr_in *addr, int port) {
    memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(port);
}


int findFreeClientIndex(pthread_t* client_tids) {
	for(int i = 0; i < MAX_CLIENTS_AMOUNT; i++) {
		if(client_tids[i] == 0) {
		    return i;
		}
	}
	return -1;
}

char *createRequest(const url_t *url) {
    char* request = (char*)malloc(sizeof(char) * (ADDRESS_BUF_SIZE + 16));
    strcpy(request, "GET /");
    strcat(request, url->path);
    strcat(request, "\r\n");
    return request;
}

void sendRequest(
        int clientSocket,
        const url_t *url) {
    if(url->path == NULL){
        write(clientSocket, "GET /\r\n", strlen("GET /\r\n"));
    } else{
        char *request = createRequest(url);
        if(DEBUG)printf("[DEBUG]: REQUEST: %s", request);
        write(clientSocket, request, strlen(request));
        free(request);
    }
}

int tryFindAtCache(const char* url) {
	for(int i = 0; i < cacheFreeIndex; i++) {
		//printf("title = %s\n", cache[i].title);
		if(strcmp(url, cache[i].title) == 0) return i;
	}
	return -1;
}

void readToCache(int cacheIndex, int clientSocket) {
    int offset = 0;
    int read_bytes = 0;
    while((read_bytes = read(clientSocket, &((cache[cacheIndex].page)[offset]), BUFFER_SIZE)) != 0) {
        offset += read_bytes;
        printf("cache[%d].page size = %d\n", cacheIndex , cache[cacheIndex].page_size);
        printf("read_bytes = %d. offset = %d\n", read_bytes, offset);
        cache[cacheIndex].page_size = offset;
        pthread_mutex_lock(&reallocOperationMutex);
        cache[cacheIndex].page = (char*)realloc(cache[cacheIndex].page, offset + BUFFER_SIZE + 1);
        pthread_mutex_unlock(&reallocOperationMutex);
    }
}

void handleClientDisconnect(int clientFd, int clientIndex) {
    printf("client %d disconnected\n", clientFd);
    clientThreads[clientIndex] = 0;
    close(clientFd);
}

void writeToClient(int clientFD, int socket){
    int read_bytes = 0;
    char buffer[BUFFER_SIZE + 1] = {0};
    while((read_bytes = read(socket, buffer, BUFFER_SIZE)) != 0) {
        if((write(clientFD, buffer, read_bytes)) < 1) {
            break;
        }
    }
}

void* clientHandler(void* arg) {
    threadArg_t * threadArg = (threadArg_t *) arg;
    int clientFd = threadArg->clientFD;
    int clientIndex = threadArg->clientIndex;
    free(arg);
    if(DEBUG) printf("[DEBUG]: new client thread created for %d\n", clientFd);
    char* urlBuffer = calloc(BUFFER_SIZE + 1, sizeof(char));
    int read_bytes = -1;
    while(1) {
		if((read_bytes = read(clientFd, urlBuffer, BUFFER_SIZE)) < 1) {
            handleClientDisconnect(clientFd, clientIndex);
            break;
		} else {
            urlBuffer[read_bytes] = '\0';
			if(DEBUG) printf("[DEBUG]: read %d bytes from %d\n", read_bytes, clientFd);
			if (strcmp(urlBuffer, "/exit") == 0){
                handleClientDisconnect(clientFd, clientIndex);
			    break;
			}
			int cacheIndex;
			if((cacheIndex = tryFindAtCache(urlBuffer)) != -1) {
				printf("Page found at cache!\n");
			} else {
			    url_t * url = parseURL(urlBuffer);
			    if (url == NULL){
			        printf("Wrong url=%s\n", urlBuffer);
                    continue;
			    }
                int clientSocket = socketConnect(url->host, url->port);
                if(clientSocket == -1) {
                    write(clientFd, "Failed connect!\n", strlen("Failed connect!\n"));
                    continue;
                }
                sendRequest(clientSocket, url);
                if(cacheFreeIndex > CACHE_SIZE - 1) {
                    write(clientFd, "Cache is full!\n", strlen("Cache is full!\n"));
                    writeToClient(clientFd, clientSocket);
                    printf("Cache is full\n");
                    continue;
                }
                pthread_mutex_lock(&cacheIndexMutex);
                cacheIndex = cacheFreeIndex++;
                pthread_mutex_unlock(&cacheIndexMutex);
                if (DEBUG) printf("Cache index = %d\n", cacheIndex);
                pthread_mutex_lock(&(cache[cacheIndex].mutex));

                cache[cacheIndex].title = (char*)malloc(strlen(urlBuffer) + 1);
                strcpy(cache[cacheIndex].title, urlBuffer);
			    readToCache(cacheIndex, clientSocket);

                pthread_mutex_unlock(&(cache[cacheIndex].mutex));
			}
			cache[cacheIndex].lastTime = time(NULL);
			if((write(clientFd, cache[cacheIndex].page, cache[cacheIndex].page_size)) < 1) {
				//pthread_mutex_unlock(&(cache[cacheIndex].mutex));
				break;
			}

		}
	}
	
	close(clientFd);
	pthread_exit((void *)1);
}

void * cacheCleaner(void * arg){
    while (1) {
        sleep(20);
        pthread_mutex_lock(&cacheIndexMutex);
        int cacheIndex = cacheFreeIndex;
        pthread_mutex_unlock(&cacheIndexMutex);
        for (int i = cacheIndex - 1; i >= 0; i--) {
            pthread_mutex_lock(&(cache[i].mutex));
            printf("[DEBUG] Cache ent check=%s index=%d lt=%ld  curTime=%ld\n", cache[i].title, i, cache[i].lastTime, time(NULL));
            if (time(NULL) - cache[i].lastTime > 30) {
                free(cache[i].page);
                cache[i].page = (char *)calloc(BUFFER_SIZE, sizeof(char));
                printf("Cache ent=%s  removed\n", cache[i].title);
                cache[i].title = NULL;
                cache[i].page_size = 0;
                pthread_mutex_lock(&cacheIndexMutex);
                cacheFreeIndex--;
                pthread_mutex_unlock(&cacheIndexMutex);
            }
            pthread_mutex_unlock(&(cache[i].mutex));
            break;
        }
    }
}


int main(int argc, char *argv[]) {
	
	pthread_mutex_init(&cacheIndexMutex, NULL);
	pthread_mutex_init(&reallocOperationMutex, NULL);

    clientThreads = (pthread_t*)calloc(MAX_CLIENTS_AMOUNT, sizeof(pthread_t));
    cache = (cache_t*)calloc(CACHE_SIZE, sizeof(cache_t));

    for(int i = 0; i < CACHE_SIZE; i++) {
        pthread_mutex_init(&(cache[i].mutex), NULL);
        cache[i].lastTime = time(NULL);
    }

    struct sockaddr_in servAddr;
    addrInit(&servAddr, PORT);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1){
        printf("Error while socket opening");
        return 1;
    }
    bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
    listen(listenfd, MAX_CLIENTS_AMOUNT);

    int newClientSFD = -1;
    pthread_t clearThread;
    pthread_create(&clearThread, NULL, cacheCleaner, NULL);

	while (1) {
		if((newClientSFD = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) {
			perror("Failed to accept client");
		} else {
			
			int freeClientIndex = findFreeClientIndex(clientThreads);
			
			if(freeClientIndex == -1) {
				printf("Can't accept clients anymore!");
				continue;
			}
			threadArg_t * threadArg = (threadArg_t *) malloc(sizeof(threadArg_t));
			threadArg->clientFD = newClientSFD;
			threadArg->clientIndex = freeClientIndex;

			while (pthread_create(&(clientThreads[freeClientIndex]), NULL, clientHandler, threadArg) != 0  && errno == EAGAIN){}
		}
	}
	return 0;
}









