#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>


sem_t semaphore1, semaphore2;

void * threadFunction(void * param){
    for (int i = 0; i < 5; ++i){
        sem_wait(&semaphore1);
        printf("Thread - %d\n", i);
        sem_post(&semaphore2);
    }
    pthread_exit(NULL);
}

int main(int argc, char ** argv) {
    pthread_t thread;
    sem_init(&semaphore1, 0, 0);
    sem_init(&semaphore2, 0, 1);
    int err = pthread_create(&thread, NULL, threadFunction, NULL);
    if (err){
        printf("Error №%d: %s\n", err, strerror(err));
    }
    for (int i = 0; i < 5; ++i){
        sem_wait(&semaphore2);
        printf("Main thread - %d\n", i);
        sem_post(&semaphore1);
    }
    pthread_exit(NULL);
}