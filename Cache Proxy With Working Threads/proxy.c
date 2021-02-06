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
#include <signal.h>

#define COUNT_OF_USER 10
#define ADDRESS_BUF_SIZE 256
#define CACHE_SIZE 3
#define BUF_SIZE 1024

typedef struct cache {
	int page_size;
	char* url;
	char* page;
	pthread_mutex_t mutex;
	long lastTime;
} cache_t;

typedef struct userArg{
    int userFd;
    int userIndex;
} userArg_t;

pthread_mutex_t cacheIndexMutex;
pthread_mutex_t reallocMutex;

pthread_t* userThreads;
cache_t * cache;
int cacheFreeMas[CACHE_SIZE] = {0};
int listenFd;

int * clients;                  
int * clientsHttpSockets;       
int * cacheToClient;            
int * sentBytes; 

int usersWithThreads = 0;

int parse_url(char *url, char *host, char *path, int *port) {
    *port = 80;
    int startPortIndex = 0;
    int url_size = strlen(url);
    for (int sl_index = 0; sl_index < url_size; sl_index++){
        if(url[sl_index] == ':'){
            startPortIndex = sl_index;
            int portIndex = sl_index + 1;
            int p = 0;
            char portStr[6] = {0};
            while(p < 5 && portIndex < url_size && isdigit(url[portIndex])){
                portStr[p++] = url[portIndex];
                portIndex++;
            }

            *port = atoi(portStr);
            if(*port > 65535){
                printf("Error: max port = 65635");
                return -1;
            }
        }
        if(url[sl_index] == '/'){
                strncpy(host, url, startPortIndex  ? startPortIndex : sl_index);
            if (sl_index + 1 == url_size) {
                strcpy(path,"/");
                break;
            }
            strncpy(path, url + sl_index, url_size - sl_index);
            if (url[url_size - 1] != '/'){
                strcat(path, "/");
            }
            break;
        }
        if (sl_index + 1 == url_size){
            strcpy(path,"/");
            strncpy(host, url, startPortIndex  ? startPortIndex : sl_index + 1);
        }

    }
    return 0;
}


int socket_connect(char *host, in_port_t server_port){

    struct hostent *hp = gethostbyname(host);
    if(hp == NULL){
        perror("gethostbyname");
        return -1;
    }
    struct sockaddr_in servAddr;
    memcpy(&servAddr.sin_addr, hp->h_addr, hp->h_length);
    servAddr.sin_port = htons(server_port);
    servAddr.sin_family = AF_INET;

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1){
        perror("socket");
        return -1;
    }
    int on = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(connect(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        return -1;
    }

    return sock;
}

void send_request(int clientSocket, char *path) {
    char* request = (char*)malloc(sizeof(char) * (ADDRESS_BUF_SIZE + 16));
    strcpy(request, "GET ");
    strcat(request, path);
    strcat(request, "\r\n");
    write(clientSocket, request, strlen(request));
    free(request);
}

int try_find_at_cache_thread(const char* url) {
	for(int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].url == NULL){
            continue;
        }
		if(strcmp(url, cache[i].url) == 0) return i;
	}
	return -1;
}

int try_find_at_cache(int userIndex, const char *urlBuffer) {
    for(int i = 0; i < CACHE_SIZE; i++) {
        pthread_mutex_lock(&(cache[i].mutex));
        if (cache[i].url == NULL){
            pthread_mutex_unlock(&(cache[i].mutex));
            continue;
        }
        if(strcmp(cache[i].url, urlBuffer) == 0) {
            pthread_mutex_unlock(&(cache[i].mutex));
            cache[i].lastTime = time(NULL);
            cacheToClient[userIndex] = i;
            sentBytes[userIndex] = 0;
            printf("Page found in cache!\n");
            return i;
        }
        pthread_mutex_unlock(&(cache[i].mutex));
    }
    return -1;
}

void read_to_cache(int cacheIndex, int clientSocket) {
    int offset = 0;
    int read_bytes = 0;
    while((read_bytes = read(clientSocket, &((cache[cacheIndex].page)[offset]), BUF_SIZE)) != 0) {
        offset += read_bytes;
        cache[cacheIndex].page_size = offset;
        pthread_mutex_lock(&reallocMutex);
        cache[cacheIndex].page = (char*)realloc(cache[cacheIndex].page, offset + BUF_SIZE + 1);
        pthread_mutex_unlock(&reallocMutex);
    }
}

void user_disconnecting(int userFd, int userIndex, int workingInThread) {
    if (workingInThread){
        printf("User %d disconnected\n", userFd);
        userThreads[userIndex] = 0;
        close(userFd);
        usersWithThreads--;
    }
    else{
        printf("User %d disconnecting...\n", userIndex);
        close(clients[userIndex]);
        close(clientsHttpSockets[userIndex]);
        clients[userIndex] = -1;
        clientsHttpSockets[userIndex] = -1;
        cacheToClient[userIndex] = -1;
        sentBytes[userIndex] = 0;
    }
    
}

void resend_from_cache(int userIndex) {
    int written_bytes = 0;
    if(cacheToClient[userIndex] != -1) {
        pthread_mutex_lock(&(cache[cacheToClient[userIndex]].mutex));
        cache[cacheToClient[userIndex]].lastTime = time(NULL);
        int bytes_left = cache[cacheToClient[userIndex]].page_size - sentBytes[userIndex];
        if (bytes_left > 0) { 
            if ((written_bytes = write(clients[userIndex], &(cache[cacheToClient[userIndex]].page[sentBytes[userIndex]]), BUF_SIZE < bytes_left ? BUF_SIZE : bytes_left)) > 0) {
                sentBytes[userIndex] += written_bytes;
            } else{
                pthread_mutex_unlock(&(cache[cacheToClient[userIndex]].mutex));
                user_disconnecting(-1, userIndex, 0);
                return;
            }
        }
        pthread_mutex_unlock(&(cache[cacheToClient[userIndex]].mutex));
    }
}

void write_to_client(int userFd, int socket){
    int read_bytes = 0;
    char buffer[BUF_SIZE + 1] = {0};
    while((read_bytes = read(socket, buffer, BUF_SIZE)) != 0) {
        if((write(userFd, buffer, read_bytes)) < 1) {
            break;
        }
    }
}

int try_accept_new_client(int listenFd,int alreadyConnectedClientsNumber,struct timeval * timeout,fd_set *lfds) {
    if(alreadyConnectedClientsNumber < COUNT_OF_USER) {
        if(select(listenFd + 1, lfds, NULL, NULL, timeout)){
            return accept(listenFd, (struct sockaddr*)NULL, NULL);
        }
    }
    return -1;
}

int find_free_cache_index(){
    for(int i = 0; i < CACHE_SIZE; i++){
        if(cacheFreeMas[i] == 0){
            return i;
        }
    }
    return -1;
}

void* client_function(void* arg) {
    userArg_t * userArg = (userArg_t *) arg;
    int userFd = userArg->userFd;
    int userIndex = userArg->userIndex;
    free(arg);

    printf("New client thread created for %d\n", userFd);
    char* urlBuffer = calloc(BUF_SIZE + 1, sizeof(char));
    int read_bytes = -1;
    while(1) {
		if((read_bytes = read(userFd, urlBuffer, BUF_SIZE)) < 1) {
            printf("%d\n", read_bytes);
            user_disconnecting(userFd, userIndex, 1);
            break;
		} 
        urlBuffer[read_bytes] = '\0';

        if (strcmp(urlBuffer, "/EXIT") == 0){
            user_disconnecting(userFd, userIndex, 1);
            break;
        }
        int cacheIndex;
        if((cacheIndex = try_find_at_cache_thread(urlBuffer)) != -1) {
            printf("Page found at cache!\n");
        } else {
            char host[1024] = {0};
            char path[1024] = {0};
            int port = 0;
            int code_port = parse_url(urlBuffer, host, path, &port);

            if(code_port == -1){
                write(userFd, "Error port!\n", strlen("Error port!\n"));
                continue;
            }
            
            int clientSocket = socket_connect(host, port);
            if(clientSocket == -1) {
                write(userFd, "Failed connect!\n", strlen("Failed connect!\n"));
                continue;
            }
            send_request(clientSocket, path);
        
            pthread_mutex_lock(&cacheIndexMutex);
            cacheIndex = find_free_cache_index();
    
            if(cacheIndex == -1) {
                pthread_mutex_unlock(&cacheIndexMutex);
                write(userFd, "Cache is full!\n", strlen("Cache is full!\n")); 
                write_to_client(userFd, clientSocket);
                printf("Cache is full\n");
                close(clientSocket);
                continue;
            }
            cacheFreeMas[cacheIndex] = 1;
            pthread_mutex_unlock(&cacheIndexMutex);


            pthread_mutex_lock(&(cache[cacheIndex].mutex));

            cache[cacheIndex].url = (char*)malloc(strlen(urlBuffer) + 1);
            strcpy(cache[cacheIndex].url, urlBuffer);
            read_to_cache(cacheIndex, clientSocket);
            close(clientSocket);
            pthread_mutex_unlock(&(cache[cacheIndex].mutex));
        }
        cache[cacheIndex].lastTime = time(NULL);
        if((write(userFd, cache[cacheIndex].page, cache[cacheIndex].page_size)) < 1) {
            break;
        }

	}
	close(userFd);
	pthread_exit((void *)1);
}

void * checking_and_clearing_cache(void * arg){
    while (1) {
        sleep(20);
        pthread_mutex_lock(&cacheIndexMutex);
        for (int i = 0; i < CACHE_SIZE; i++) {
            if(cacheFreeMas[i] == 0){
                continue;
            }
            pthread_mutex_lock(&(cache[i].mutex));
            if (time(NULL) - cache[i].lastTime > 30) {
                free(cache[i].page);
                cache[i].page = (char *)calloc(BUF_SIZE, sizeof(char));
                printf("Cache url=%s  removed\n", cache[i].url);
                cache[i].url = NULL;
                cache[i].page_size = 0;
                cacheFreeMas[i] = 0;
            }
            pthread_mutex_unlock(&(cache[i].mutex));
        }
        pthread_mutex_unlock(&cacheIndexMutex);
    }
}

void creating_server_handle(int server_port, int listenFd){
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(struct sockaddr_in));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(server_port);

    if (listenFd == -1){
        printf("Error while socket opening");
        exit(-1);
    }
    bind(listenFd, (struct sockaddr*)&servAddr, sizeof(servAddr));
    listen(listenFd, COUNT_OF_USER);

}

void init_mutex_function(){

    pthread_mutex_init(&cacheIndexMutex, NULL);
	pthread_mutex_init(&reallocMutex, NULL);

    cache = (cache_t*)calloc(CACHE_SIZE, sizeof(cache_t));

    for(int i = 0; i < CACHE_SIZE; i++) {
        pthread_mutex_init(&(cache[i].mutex), NULL);
        cache[i].lastTime = time(NULL);
    }
}

void get_signal(int signo){
    close(listenFd);
    exit(-1);
}


int main(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Invalid number of arguments"); 
        return 1;
    }
    signal(SIGINT,get_signal);
	init_mutex_function();


    int server_port = atoi(argv[1]);
    int countOfTreads = atoi(argv[2]);

    if(countOfTreads > COUNT_OF_USER){
        printf("Count of user must be less than %d", COUNT_OF_USER);
        exit(-1);
    }

    listenFd = socket(AF_INET, SOCK_STREAM, 0);

    shutdown(listenFd,2);

    creating_server_handle(server_port, listenFd);

    int countWithoutThreads = COUNT_OF_USER - countOfTreads;
    int userWithoutThreads = 0;

    clients = (int *)malloc(countWithoutThreads * sizeof(int));                  
    clientsHttpSockets = (int *)malloc(countWithoutThreads * sizeof(int));       
    cacheToClient = (int *)malloc(countWithoutThreads * sizeof(int));            
    sentBytes = (int *)malloc(countWithoutThreads * sizeof(int)); 

    for(int k = 0; k < countWithoutThreads; k++) {
        clients[k] = -1;
        clientsHttpSockets[k] = -1;
        cacheToClient[k] = -1;
        sentBytes[k] = -1;
    }

    pthread_t clearThread;
    pthread_create(&clearThread, NULL, checking_and_clearing_cache, NULL);

    userThreads = (pthread_t*)calloc(countOfTreads, sizeof(pthread_t));
    int userFd = -1;
    int countOfconnectedUser = 0;
    int cacheIndex = -1;

    

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set lfds;
    fd_set cfds;


	while (1) {
        FD_ZERO(&lfds);
        FD_SET(listenFd, &lfds);

        userFd = try_accept_new_client(listenFd, countOfconnectedUser, &timeout, &lfds);

        if (userFd != -1){
            countOfconnectedUser++;
            if (usersWithThreads < countOfTreads){
                int freeNumTreadForUser = -1;
                for(int i = 0; i < countOfTreads; i++) {
                    if(userThreads[i] == 0) {
                        freeNumTreadForUser = i;
                        break;
                    }
                }

                if(freeNumTreadForUser == -1) {
                    printf("Can't accept clients anymore!\n");
                    continue;
                }
                userArg_t * userArg = (userArg_t *)malloc(sizeof(userArg_t));
                userArg->userFd = userFd;
                userArg->userIndex = freeNumTreadForUser;
                usersWithThreads++;
                while (pthread_create(&(userThreads[freeNumTreadForUser]), NULL, client_function, userArg) != 0  && errno == EAGAIN){}
                
            }  else{
                int i = 0;
                for (i = 0; i < countWithoutThreads; i++){
                    if (clients[i] == -1){
                        clients[i] = userFd;
                        break;
                    }
                }
                if (i == countWithoutThreads){
                    printf("Cant accept client\n");
                    char * errorMsg = "Cant accept new client. Sorry. Try later\n";
                    write(userFd, errorMsg, strlen(errorMsg));
                    close(userFd);
                }
                printf("Client without thread has index = %d\n", i);
            }
        }
		
        for(int userIndex = 0; userIndex < countWithoutThreads; userIndex++) {
        
            if(clients[userIndex] == -1) continue;

            FD_ZERO(&cfds);
            FD_SET(clients[userIndex], &cfds);

            if(clientsHttpSockets[userIndex] == -1 && select(clients[userIndex] + 1, &cfds, NULL, NULL, &timeout)) {
                char urlBuffer[ADDRESS_BUF_SIZE];
                int read_bytes = read(clients[userIndex], &urlBuffer, ADDRESS_BUF_SIZE);
                if (read_bytes < 1){
                    user_disconnecting(0, userIndex, 0);
                    continue;
                }
                urlBuffer[read_bytes] = '\0';

                if(strcmp(urlBuffer, "/EXIT") == 0) {
                    countOfconnectedUser--;
                    user_disconnecting(-1, userIndex, 0);
                    continue;
                }
                char host[1024] = {0};
                char path[1024] = {0};
                int port = 0;
                int code_port = parse_url(urlBuffer, host, path, &port);

                if(code_port == -1){
                    write(clients[userIndex], "Error port!\n", strlen("Error port!\n"));
                    continue;
                }

                cacheIndex = try_find_at_cache(userIndex, urlBuffer);
                if (cacheIndex != -1){
                    printf("URL = %s  found at cache with index =%d\n", urlBuffer, cacheIndex);
                }
                if (cacheIndex == -1) {
                    clientsHttpSockets[userIndex] = socket_connect(host, port);
                    printf("Try make connection to host=%s  with path = %s  with port = %d\n", host, path, port);
                    printf("socket_connect return = %d\n", clientsHttpSockets[userIndex]);
                    if(clientsHttpSockets[userIndex] == -1) {
                        write(clients[userIndex], "Failed connect!\n", strlen("Failed connect!\n"));
                        continue;
                    }

                    send_request(clientsHttpSockets[userIndex], path);
                    pthread_mutex_lock(&cacheIndexMutex);
                    cacheIndex = find_free_cache_index();
                    if (cacheIndex == -1){
                        pthread_mutex_unlock(&cacheIndexMutex);
                        printf("Cache is full\n");
                        write_to_client(clients[userIndex], clientsHttpSockets[userIndex]);
                        close(clientsHttpSockets[userIndex]);
                        clientsHttpSockets[userIndex] = -1;
                        continue;
                    }
                    cacheFreeMas[cacheIndex] = 1;
                    pthread_mutex_unlock(&cacheIndexMutex);
                    pthread_mutex_lock(&(cache[cacheIndex].mutex));
                    cache[cacheIndex].url = (char*)malloc(sizeof(char) * strlen(urlBuffer));
                    strcpy(cache[cacheIndex].url, urlBuffer);

                    cache[cacheIndex].page_size = BUF_SIZE;
                    cache[cacheIndex].page = (char*)malloc(BUF_SIZE + 1);

                    cacheToClient[userIndex] = cacheIndex;
                    sentBytes[userIndex] = 0;
                    printf("Try read url=%s  user=%d to cache with index=%d\n", urlBuffer, userIndex, cacheIndex);
                    read_to_cache(cacheIndex, clientsHttpSockets[userIndex]);
                    cache[cacheIndex].lastTime = time(NULL);
                    close(clientsHttpSockets[userIndex]);
                    clientsHttpSockets[userIndex] = -1;
                    cache[cacheIndex].page[cache[cacheIndex].page_size + 1] = '\0';

                    pthread_mutex_unlock(&(cache[cacheIndex].mutex));
                }

            }
            resend_from_cache(userIndex);
        }
        
        
	}
	return 0;
}
