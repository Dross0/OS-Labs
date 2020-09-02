#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

void cleanup_handler(void * param){
    printf("\nThread canceled. Message from thread\n");
}

void * thread_print(void * param){
    pthread_cleanup_push(cleanup_handler, NULL);
    char symbol = 'A';
    while (1){
        pthread_testcancel();
        printf("%c", symbol);
        if (++symbol == 'z'){
            symbol = 'A';
        }
    }
    pthread_cleanup_pop(1);  // 0 - при извлечении не исполнять обработчик
}

int main() {
    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_print, NULL);
    if (err){
        printf("Error №%d: %s\n", err, strerror(err));
    }
    sleep(2);
    pthread_cancel(thread);
    pthread_join(thread, NULL);
    return 0;
}