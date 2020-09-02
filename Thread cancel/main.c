#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

void * thread_print(void * param){
    char symbol = 'A';
    while (1){
        pthread_testcancel();
        printf("%c", symbol);
        if (++symbol == 'z'){
            symbol = 'A';
        }
    }
}

int main() {
    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_print, NULL);
    if (err){
        printf("Error №%d: %s\n", err, strerror(err));
    }
    sleep(2);
    pthread_cancel(thread);
    printf("\nThread canceled\n");
    pthread_join(thread, NULL);
    return 0;
}