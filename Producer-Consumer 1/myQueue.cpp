#include "myQueue.h"
#include <cstring>
#include <string>
#define MAX_FULL 10
#define MAX_SIZE_STR 80

MyQueue::MyQueue() {
    dropped = false;
    sem_init(&full, 0, MAX_FULL);
    sem_init(&empty, 0, 0);
    pthread_mutex_init(&block, NULL);
}

MyQueue::~MyQueue() {
    sem_destroy(&full);
    sem_destroy(&empty);
    pthread_mutex_destroy(&block);
}

bool MyQueue::isDropped(){
    return dropped;
}

int MyQueue::mymsgget(char *buf, size_t bufsize) {
      
    sem_wait(&empty);
    pthread_mutex_lock(&block);

    if(dropped) {
        sem_post(&empty);
        pthread_mutex_unlock(&block);
        return 0;
    }
    char *tmp = storage.front();
    storage.pop();
    tmp[bufsize] = 0;
    strcpy(buf, tmp);
    free(tmp);

    sem_post(&full);
    pthread_mutex_unlock(&block);
    
    return  strlen(buf);
}

int MyQueue::mymsgput(const char *msg) {
        
    sem_wait(&full);
    pthread_mutex_lock(&block);

    if(dropped) {
        sem_post(&full);
        pthread_mutex_unlock(&block);
        return 0;
    }

    size_t str_size = strlen(msg);
    int msg_len = str_size > MAX_SIZE_STR ? MAX_SIZE_STR : str_size;

    char* str = (char*)calloc(msg_len + 1, sizeof(char));

    strncpy(str, msg, msg_len);
    str[msg_len] = 0;

    storage.push(str);

    sem_post(&empty);
    pthread_mutex_unlock(&block);
    return msg_len;
}

void MyQueue::mymsqdrop() {
    pthread_mutex_lock(&block);
    dropped = true;
    sem_post(&empty);
    sem_post(&full);
    pthread_mutex_unlock(&block);
}

