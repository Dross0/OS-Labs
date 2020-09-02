#include <stdio.h>
#include <pthread.h>
#include <string.h>


void * printStringArray(void * param){
    char ** stringArray = (char **) param;
    for (char ** currentString = stringArray; *currentString != NULL; ++currentString){
        printf("%s\n", *currentString);
    }
}

void errorHandling(int err){
    if (err){
        printf("Error №%d: %s\n", err, strerror(err));
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
    int err = pthread_create(&thread1, NULL, printStringArray, stringArray1);
    errorHandling(err);
    err = pthread_create(&thread2, NULL, printStringArray, stringArray2);
    errorHandling(err);
    err = pthread_create(&thread3, NULL, printStringArray, stringArray3);
    errorHandling(err);
    err = pthread_create(&thread4, NULL, printStringArray, stringArray4);
    errorHandling(err);
    pthread_exit(NULL);
}