#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char * argv[]){
    int rv = 0;
    pid_t pid = fork();
    switch (pid){
        case -1:
            fprintf(stderr, "Ошибка в создании процесса потомка\n");
            break;
        case 0:
            system("cat long_file.txt");
        default:
            printf("Hello, world\n");
    }
    pid = fork();
    switch (pid){
        case -1:
            fprintf(stderr, "Ошибка в создании процесса потомка\n");
        case 0:
            system("cat long_file.txt");
            exit(rv);
        default:
            wait();
            printf("Процесс потомок вернул - %d\n", WEXITSTATUS(rv));
    }
}