#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex;
pthread_cond_t condVar;

int isMainOutput = 1;

void * threadFunction(void * param){
    for (int i = 0; i < 5; ++i){
        pthread_mutex_lock(&mutex);
        while (isMainOutput){
            pthread_cond_wait(&condVar, &mutex);
        }
        isMainOutput = 1;
        printf("Thread - %d\n", i);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&condVar);
    }
    pthread_exit(NULL);
}

int main(int argc, char ** argv) {
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condVar, NULL);
    int err = pthread_create(&thread, NULL, threadFunction, NULL);
    if (err){
        printf("Error №%d: %s\n", err, strerror(err));
    }
    for (int i = 0; i < 5; ++i){
        pthread_mutex_lock(&mutex);
        while (!isMainOutput){
            pthread_cond_wait(&condVar, &mutex);
        }
        isMainOutput = 0;
        printf("Main thread - %d\n", i);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&condVar);
    }
    pthread_exit(NULL);
}