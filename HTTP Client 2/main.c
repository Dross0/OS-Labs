#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/termios.h>
#include <aio.h>
#include <ctype.h>

#define WRONG_ARGUMENTS 1
#define CANT_PARSE_URL 2
#define CONNECT_FAILURE 3
#define SOCKET_INPUT_ERROR 4
#define STDIN_INPUT_ERROR 5
#define POLL_FAILURE 6
#define SEND_FAILURE 7

#define POLL_SIZE 2
#define REQUEST_SIZE 256
#define ONE_LINE_SIZE 80
#define LINES_ON_SCREEN 25
#define MAX_BUFFERS 100

#define SOCKET_IND 1
#define STDIN_IND 0

#define TIMEOUT 10000

char ** buffers;
int bufSize = MAX_BUFFERS;

int current_buffer = -1;
char * c;
int user_current_buffer = -1;
struct termios old;
int sockfd = 0;


aiocb_t http_aio;
aiocb_t term_aio;

int add_new_buffer(char ** buffers, int ind, size_t size){
    int cur = ind;
    ind++;
	if (ind == bufSize){
		bufSize *= 2;
		buffers = realloc(buffers, bufSize);
	}
    buffers[ind] = (char *)malloc(sizeof(char) * size);
    return ind;
}

void free_buffers(char ** buffers){
    int i = 0;
    for (i = 0; i < bufSize; ++i){
        free(buffers[i]);
    }
	free(buffers);
}

void finish(){
    free_buffers(buffers);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    exit(0);
}


void term_function() {
	if(*c == ' '){
		if(++user_current_buffer > current_buffer){
			printf("END!\n");
			finish();
		} 
		printf("%s\n", buffers[user_current_buffer]);
		
	} else if(*c == 'q'){
		finish();
	}
	
	aio_read((void*)&term_aio);
}

void http_function() {
	
	int ret = aio_return((void*)&http_aio);	
	if(ret < 1) {
		close(sockfd);
		return;
	
	}
	
	current_buffer = add_new_buffer(buffers, current_buffer, LINES_ON_SCREEN * ONE_LINE_SIZE);
	
	http_aio.aio_buf = buffers[current_buffer];
	
	aio_read((void*)&http_aio);
}


int main(int argc, char ** argv){
    if (argc < 2 || argc > 3){
        printf("Wrong arguments amount\n");
        return WRONG_ARGUMENTS;
    }

buffers = (char **)calloc(bufSize, sizeof(char*));

struct addrinfo hints;
struct addrinfo * res;
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;

char host[1024] = {0};
char port[6] = {0};
strcpy(port,"80");
char path[1024] = {0};
int startPortIndex = 0;
int url_size = strlen(argv[1]);
for (int sl_index = 0; sl_index < url_size; sl_index++){
    if(argv[1][sl_index] == ':'){
        startPortIndex = sl_index;
        int portIndex = sl_index + 1;
        int p = 0;
        memset(port,0,6);
        while(p < 5 && portIndex < url_size && isdigit(argv[1][portIndex])){
            port[p++] = argv[1][portIndex];
            portIndex++;
        }
    }
    if(argv[1][sl_index] == '/'){
            strncpy(host, argv[1], startPortIndex  ? startPortIndex : sl_index);
        if (sl_index + 1 == url_size) {
            strcpy(path,"/");
            break;
        }
        strncpy(path, argv[1] + sl_index + 1, url_size - sl_index - 1);
        break;
    }
    if (sl_index + 1 == url_size){
             strcpy(path,"/");
             strncpy(host, argv[1], startPortIndex  ? startPortIndex : sl_index + 1);
    }

}

if (getaddrinfo(host, port, &hints, &res) != 0){
    perror("Getaddrinfo");
    return CANT_PARSE_URL;
}

struct addrinfo * p;
for (p = res; p != NULL; p = p->ai_next){
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    shutdown(sockfd,2);
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1){
        break;
    }
    close(sockfd);
}
if (p == NULL){
    fprintf(stderr, "Cant connect\n");
    return CONNECT_FAILURE;
}
freeaddrinfo(res);
char request[REQUEST_SIZE] = {0};
snprintf(request, REQUEST_SIZE, "GET %s HTTP/1.1\nHost: %s\n\n", path, host); 
if (send(sockfd, request, strlen(request), 0) == -1){
    perror("Send");
    return SEND_FAILURE;
}

c = (char *)malloc(sizeof(char));

term_aio.aio_fildes = STDIN_FILENO;
term_aio.aio_buf = c;
term_aio.aio_nbytes = 1;
term_aio.aio_offset = 0;
term_aio.aio_sigevent.sigev_notify = SIGEV_THREAD;
term_aio.aio_sigevent.sigev_notify_function = term_function;
term_aio.aio_sigevent.sigev_notify_attributes = NULL;

aio_read((void*)&term_aio);

struct termios new;
tcgetattr(STDIN_FILENO, &old);
new = old;
new.c_lflag &= ~(ICANON | ECHO);
new.c_cc[VMIN] = 1;
tcsetattr(STDIN_FILENO, TCSANOW, &new);

current_buffer = add_new_buffer(buffers, current_buffer, LINES_ON_SCREEN * ONE_LINE_SIZE);
http_aio.aio_fildes = sockfd;
http_aio.aio_buf = buffers[current_buffer];
http_aio.aio_nbytes = LINES_ON_SCREEN * ONE_LINE_SIZE;
http_aio.aio_offset = 0;
http_aio.aio_sigevent.sigev_notify = SIGEV_THREAD;
http_aio.aio_sigevent.sigev_notify_function = http_function;
http_aio.aio_sigevent.sigev_notify_attributes = NULL;

aio_read((void*)&http_aio);

while(1){}

return 0;
}
