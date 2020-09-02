#include <stdio.h>
#include <pthread.h>

void * printStringArray(void * param){
    char ** stringArray = (char **) param;
    for (char ** currentString = stringArray; *currentString != NULL; ++currentString){
        printf("%s\n", *currentString);
    }
}

int main(int argc, char ** argv) {
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    pthread_t thread4;
    char * stringArray1[] = {"1", "2", "3", NULL};
    char * stringArray2[] = {"4", "5", "6", "7", "8", NULL};
    char * stringArray3[] = {"9", "10", NULL};
    char * stringArray4[] = {"11", "12", "13", NULL};
    if (pthread_create(&thread1, NULL, printStringArray, &stringArray1) != 0){
        perror("Thread 1 creation error");
    }
    pthread_join(thread1, NULL);
    if (pthread_create(&thread2, NULL, printStringArray, stringArray2) != 0){
        perror("Thread 2 creation error");
    }
    if (pthread_create(&thread3, NULL, printStringArray, stringArray3) != 0){
        perror("Thread 3 creation error");
    }
    if (pthread_create(&thread4, NULL, printStringArray, stringArray4) != 0){
        perror("Thread 4 creation error");
    }
    pthread_exit(NULL);
}