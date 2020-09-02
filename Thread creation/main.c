#include <stdio.h>
#include <pthread.h>

void * threadFunction(void * param){
    printf("6\n");
    printf("7\n");
    printf("8\n");
    printf("9\n");
    printf("10\n");
}

int main(int argc, char ** argv) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, threadFunction, NULL) != 0){
        perror("Thread creation");
    }
    printf("1\n");
    printf("2\n");
    printf("3\n");
    printf("4\n");
    printf("5\n");
    pthread_exit(NULL);
}