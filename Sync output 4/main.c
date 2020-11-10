#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define PRINT_TIMES 15


int main(int argc, char ** argv) {
    pid_t pid = 0;
    sem_t * sem1 = sem_open("sem1", O_CREAT, 0600, 0);
    sem_t * sem2 = sem_open("sem2", O_CREAT, 0600, 1);
    switch ((pid = fork()))
    {
    case -1:
        printf("Error fork");
        return 2;
    case 0:
        for (int i = 0; i < PRINT_TIMES; ++i){
            sem_wait(sem1);
            printf("Thread - %d\n", i);
            sem_post(sem2);
        }
        break;
    default:
        for (int i = 0; i < PRINT_TIMES; ++i){
            sem_wait(sem2);
            printf("Main thread - %d\n", i);
            sem_post(sem1);
        }
        sem_close(sem1);
        sem_close(sem2);
        break;
    }
}