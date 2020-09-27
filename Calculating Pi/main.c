#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define NUM_STEPS 20000000

typedef struct threadArgument{
    int startIndex;
    int elemsNum;
    double result;
} threadArgument_t;

void * countPartPI(void * param){
    threadArgument_t * threadParams = (threadArgument_t *) param;
    double pi = 0.0;
    int numSteps = threadParams->startIndex + threadParams->elemsNum;
    for (int i = threadParams->startIndex; i < numSteps; ++i){
        pi += 1.0 / (i * 4.0 + 1.0);
        pi -= 1.0 / (i * 4.0 + 3.0);
    }
    pi *= 4;
    threadParams->result = pi;
    printf("%d  = %.10f\n", numSteps, pi);
    pthread_exit(NULL);
}

int main(int argc, char ** argv){
    if (argc != 2){
        printf("Wrong argument amount: %d\n", argc);
        return 1;
    }
    int pthreadCount = atoi(argv[1]);
    if (pthreadCount <= 0){
        printf("Wrong argument: %s\n", argv[1]);
        return 2;
    }
    pthread_t * threads = (pthread_t *)malloc(pthreadCount * sizeof(pthread_t));
    threadArgument_t * numStepsForThread = (threadArgument_t *)calloc(pthreadCount, sizeof(threadArgument_t));
    numStepsForThread[0].startIndex = 0;
    int error = 0;
    for (int i = 0; i < pthreadCount; ++i){
        if (i > 0){
            numStepsForThread[i].startIndex = numStepsForThread[i-1].startIndex + numStepsForThread[i - 1].elemsNum;
        }
        numStepsForThread[i].elemsNum = (NUM_STEPS / pthreadCount) + ((i < NUM_STEPS % pthreadCount) ? (1) : (0));
        if ((error = pthread_create(&threads[i], NULL, countPartPI, &numStepsForThread[i]))){
            printf("Error №%d: %s\n", error, strerror(error));
            return 3;
        }
    }
    double pi = 0;
    for (int i = 0; i < pthreadCount; ++i){
        if ((error = pthread_join(threads[i], NULL))){
            printf("Error №%d: %s\n", error, strerror(error));
            return 4;
        }
        pi += numStepsForThread[i].result;
    }
    printf("PI = %.10f\n", pi);
    free(threads);
    free(numStepsForThread);
    return 0;
}