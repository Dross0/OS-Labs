#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char ** argv){
    if (argc < 2){
        fprintf(stderr, "Invalid argiment number\n");
        return 1;
    }
    pid_t pid = fork();
    int return_value = 0;
    if (pid == 0){
        execvp(argv[1], &argv[1]);
        fprintf(stderr, "Wrong command name\n");
        exit(-1);
    }
    else if (pid == -1){
        fprintf(stderr, "Cant fork\n");
    }
    if (wait(&return_value) == -1){
        perror("wait");
        return -1;
    }
    printf("\nRV = %d\n", WEXITSTATUS(return_value));
    return 0;
}