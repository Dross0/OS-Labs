#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void * thread_print(void * param){
    char symbol = 'A';
    while (1){
        printf("%c", symbol);
        if (++symbol == 'z'){
            symbol = 'A';
        }
        pthread_testcancel();
    }
}

int main() {
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_print, NULL) != 0){
        perror("Thread create");
    }
    sleep(2);
    pthread_cancel(thread);
    printf("\nThread canceled\n");
    pthread_join(thread, NULL);
    return 0;
}