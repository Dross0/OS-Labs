#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex1, mutex2;

void * threadFunction(void * param){
    for (int i = 0; i < 5; ++i){
        pthread_mutex_lock(&mutex2);
        printf("Thread - %d\n", i);
        pthread_mutex_unlock(&mutex1);
    }
    pthread_exit(NULL);
}

int main(int argc, char ** argv) {
    pthread_t thread;
    //Неясно какой поток захватит мьютекс первым - поэтому решение с 2-мя мьютексами не подходит
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);
    int err = pthread_create(&thread, NULL, threadFunction, NULL);
    if (err){
        printf("Error №%d: %s\n", err, strerror(err));
    }
    sleep(1);
    pthread_mutex_lock(&mutex2);
    for (int i = 0; i < 5; ++i){
        pthread_mutex_lock(&mutex1);
        printf("Main thread - %d\n", i);
        pthread_mutex_unlock(&mutex2);
    }
    pthread_exit(NULL);
}