#include <iostream>
#include "myQueue.h"
#include <pthread.h>
#include <iostream>
#include <time.h>
#include <unistd.h>
#define BUFFER_SIZE 80
#define SLEEP 5

MyQueue q;

pthread_t producer1;
pthread_t producer2;
pthread_t consumer1;
pthread_t consumer2;


void *consumer(void* arg) {
    while(!q.isDropped()) {

        char* chr = (char*)calloc(BUFFER_SIZE + 1, sizeof(char));

        int result = q.mymsgget(chr, BUFFER_SIZE);

        std::cout << "Result is " << result << std::endl;

        std::cout << "Got " << chr << std::endl;

        free(chr);

        if(result == 0){
            pthread_exit((void*)NULL);
        }
    }

    pthread_exit((void*)NULL);
}

void *producer(void* arg) {
    while(!q.isDropped()) {
        std::string s = "";
        for(int i = 0; i < BUFFER_SIZE; i++) {
            s += ((rand() % ('z' - 'A')) + 'A');
        }

        std::cout << "Put " << s << std::endl;

        int result = q.mymsgput(s.c_str());

        if(result == 0){
            pthread_exit((void*)NULL);
        }
    }

    pthread_exit((void*)NULL);
}

void initialization_of_threads(){
    pthread_create(&producer1, NULL, producer, NULL);
    pthread_create(&producer2, NULL, producer, NULL);
    pthread_create(&consumer1, NULL, consumer, NULL);
    pthread_create(&consumer2, NULL, consumer, NULL);
}

void join_thread(){
    pthread_join(producer1, NULL);
    pthread_join(producer2, NULL);
    pthread_join(consumer1, NULL);
    pthread_join(consumer2, NULL);
}

int main() {

    initialization_of_threads();

    sleep(SLEEP);

    q.mymsqdrop();

    join_thread();

    std::cout << "DROP!!!!!  " << std::endl;

    return 0;
}

