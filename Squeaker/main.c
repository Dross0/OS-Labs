#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>



int COUNT = 0;

void handler(int signno){
    printf("Count = %d\n", COUNT);
    exit(0);
}

void hand(int s){
    COUNT++;
    printf("%c", 7);
    signal(SIGINT, hand);
}

int main(int argc, char ** argv){
    signal(SIGQUIT, handler);
    signal(SIGINT, hand);
    while(1){
        getchar();
    };
    return 0;
}
