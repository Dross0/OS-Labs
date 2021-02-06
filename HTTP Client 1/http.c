#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/termios.h>

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

int add_new_buffer(char ** buffers, int ind, size_t size){
    int cur = ind;
    ind++;
    ind %= MAX_BUFFERS;
    if (buffers[ind] == NULL){
        buffers[ind] = (char *)malloc(sizeof(char) * size);
        return ind;
    }
    return cur;
}

void free_buffers(char ** buffers){
    int i = 0;
    for (i = 0; i < MAX_BUFFERS; ++i){
        free(buffers[i]);
    }
}

void clear_before_exit(char ** buffers, struct termios * old){
    free_buffers(buffers);
    tcsetattr(STDIN_FILENO, TCSANOW, old);
}

int main(int argc, char ** argv){
    if (argc != 2){
        fprintf(stderr, "Wrong arguments amount\n");
        return WRONG_ARGUMENTS;
    }

    struct addrinfo hints;
    struct addrinfo * res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char host[1024] = {0};
    int sl_index = 0;
    int url_size = strlen(argv[1]);
    for (; sl_index < url_size && argv[1][sl_index] != '/'; sl_index++);
    strncpy(host, argv[1], sl_index * sizeof(char));
    char * path = argv[1] + sl_index;
    if (sl_index == url_size){
        path = "/";
    }
    if (getaddrinfo(host, "http", &hints, &res) != 0){
        perror("Getaddrinfo");
        return CANT_PARSE_URL;
    }
    int sockfd = 0;
    struct addrinfo * p;
    for (p = res; p != NULL; p = p->ai_next){
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
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
    
    char * buffers[MAX_BUFFERS] = {0};
    struct pollfd poll_set[POLL_SIZE] = {0};
    poll_set[SOCKET_IND].fd = sockfd;
    poll_set[SOCKET_IND].events = POLLIN;
    poll_set[STDIN_IND].fd = fileno(stdin);
    poll_set[STDIN_IND].events = POLLIN;
    int current_buffer = -1;
    int user_current_buffer = 0;
    int numfds = POLL_SIZE;
    int prevBuffer = 0;
    
    int socketIsOpen = 1;
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
   
    while(1){
        if (poll(poll_set, numfds, TIMEOUT) == -1){
            perror("poll");
            clear_before_exit(buffers, &old);
            return POLL_FAILURE;
        }
        if (socketIsOpen && poll_set[SOCKET_IND].revents & POLLIN){
            prevBuffer = current_buffer;
            int isReadable = 1;
            if ((current_buffer = add_new_buffer(buffers, current_buffer, LINES_ON_SCREEN * ONE_LINE_SIZE)) == prevBuffer){
                isReadable = 0;
            }
            int r = -1;
            if (isReadable && ((r = recv(poll_set[SOCKET_IND].fd, buffers[current_buffer], LINES_ON_SCREEN * ONE_LINE_SIZE, 0)) == -1)){
                perror("Recv");
                clear_before_exit(buffers, &old);
                return SOCKET_INPUT_ERROR;
            }
            if (r == 0){
                close(poll_set[SOCKET_IND].fd);
                poll_set[SOCKET_IND].events = 0;
                poll_set[SOCKET_IND].fd = -1;
                numfds--;
                socketIsOpen = 0;
            }
        }
        if (poll_set[STDIN_IND].revents & POLLIN){
            char c = 0;
            if (read(0, &c, 1) == -1){
                perror("Read");
                clear_before_exit(buffers, &old);
                return STDIN_INPUT_ERROR;
            }
            if (c == ' '){
                if (!socketIsOpen && buffers[(user_current_buffer + 1) % MAX_BUFFERS] == NULL){
                    printf("END\n");
                    break;
                }
                if (buffers[user_current_buffer]){
                    printf("%s\n", buffers[user_current_buffer]);
                    free(buffers[user_current_buffer]);
                    buffers[user_current_buffer] = NULL;
                    user_current_buffer++;
                    user_current_buffer %= MAX_BUFFERS;
                }
            }
            else if (c == 'q'){
                close(sockfd);
                break;
            }
        }
    }
    clear_before_exit(buffers, &old);
    return 0;
}
