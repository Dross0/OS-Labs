#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int main(int argc, char ** argv){
    if (argc != 2){
        fprintf(stderr, "Wrong arguments numbers\n");
        return 2;
    }
    int fd[2];
    if (pipe(fd) == -1){
        perror("Pipe");
        return 1;
    }
    if (fork() == 0){
        close(fd[0]);
        if (write(fd[1], argv[1], strlen(argv[1])) == -1){
            perror("Write");
            exit(1);
        }
        exit(0);
    }
    if (fork() == 0){
        close(fd[1]);
        char buffer[BUFFER_SIZE] = {0};
        int len = 0;
        while((len = read(fd[0], buffer, BUFFER_SIZE)) != 0){
            if (len == -1){
                perror("Read");
                exit(1);
            }
            for (int i = 0; i < len; i++){
                buffer[i] = toupper(buffer[i]);
            }
            printf("%s", buffer);
        }
        printf("\n");
        exit(0);
    }
    close(fd[0]);
    close(fd[1]);
    while (wait(NULL) != -1);
    return 0;
}