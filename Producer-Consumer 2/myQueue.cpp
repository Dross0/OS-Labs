#include "myQueue.h"
#include <cstring>
#include <string>
#define MAX_FULL 10
#define MIN_FULL 1
#define MAX_SIZE_STR 80

MyQueue ::MyQueue () {
    dropped = false;
    pthread_mutex_init(&block, NULL);
    pthread_mutex_init(&get_mutex, NULL);
    pthread_mutex_init(&put_mutex, NULL);
    pthread_cond_init(&new_element, NULL);
    pthread_cond_init(&empty_element, NULL);
}

MyQueue ::~MyQueue () {
    pthread_mutex_destroy(&block);
    pthread_mutex_destroy(&get_mutex);
    pthread_mutex_destroy(&put_mutex);
    pthread_cond_destroy(&new_element);
    pthread_cond_destroy(&empty_element);
}

bool MyQueue ::isDropped(){
    return dropped;
}

int MyQueue ::mymsgget(char *buf, size_t bufsize) {
      
    pthread_mutex_lock(&get_mutex);
    pthread_mutex_lock(&block);
   
    while(storage.empty() && !dropped) {
        pthread_cond_wait(&new_element, &block);
    }

     if(dropped) {
        pthread_mutex_unlock(&block);
        pthread_mutex_unlock(&get_mutex);
        pthread_cond_signal(&empty_element);
        return 0;
    }

    char *tmp = storage.front();
    storage.pop();

    tmp[bufsize] = 0;
    strcpy(buf, tmp);
    free(tmp);
   
    if(storage.size() == MAX_FULL - 1){
        pthread_cond_signal(&empty_element);
    }
	
    pthread_mutex_unlock(&block);

    pthread_mutex_unlock(&get_mutex);
    return  strlen(buf);
}

int MyQueue ::mymsgput(const char *msg) {

    pthread_mutex_lock(&put_mutex);
    pthread_mutex_lock(&block);
 
    while(storage.size() == MAX_FULL && !dropped) {
        pthread_cond_wait(&empty_element, &block);
    }

    if(dropped) {
        pthread_mutex_unlock(&block);
        pthread_mutex_unlock(&put_mutex);
        pthread_cond_signal(&new_element);
        return 0;
    }

        
    size_t str_size = strlen(msg);
    int msg_len = str_size > MAX_SIZE_STR ? MAX_SIZE_STR : str_size;

    char* str = (char*)calloc(msg_len + 1, sizeof(char));
    strncpy(str, msg, msg_len);
//    str[msg_len] = 0;

    storage.push(str);

    if(storage.size() == MIN_FULL){
        pthread_cond_signal(&new_element);
    }
    
    pthread_mutex_unlock(&block);

    pthread_mutex_unlock(&put_mutex);
    return msg_len;
}

void MyQueue ::mymsqdrop() {
    pthread_mutex_lock(&block);
    dropped = true;
    pthread_cond_signal(&empty_element);
    pthread_cond_signal(&new_element);
    pthread_mutex_unlock(&block);
}
