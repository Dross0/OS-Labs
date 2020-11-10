#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define TASK_BLOCK_SIZE 1000

static pthread_barrier_t barrier;
static int exit_flag = 0;
static int barrierIsDestroyed = 0;
pthread_mutex_t mutex;

typedef struct threadArgument{
    long startIndex;
    long stepSize;
    double result;
} threadArgument_t;

void * countPartPI(void * param){
    threadArgument_t * threadParams = (threadArgument_t *) param;
    double pi = 0.0;
    long taskIndex = threadParams->startIndex;
    long threadIterCount = 0;
    int status = 0;
    while (!exit_flag){
        
        for (long i = taskIndex; i < taskIndex + TASK_BLOCK_SIZE; i++, threadIterCount++){
            pi += 1.0 / (i * 4.0 + 1.0);
            pi -= 1.0 / (i * 4.0 + 3.0);
        }
        taskIndex += threadParams->stepSize;
        printf("1 block for %d thread\n", threadParams->startIndex / TASK_BLOCK_SIZE);
        status = pthread_barrier_wait(&barrier);
        if (status != 0 && status != PTHREAD_BARRIER_SERIAL_THREAD){
           printf("error wait barrier in thread with status = %d\n", status);
           exit(5);
       }
    }
    if (!barrierIsDestroyed){
        usleep(1);
        pthread_mutex_lock(&mutex);
        barrierIsDestroyed = 1;
        pthread_mutex_unlock(&mutex);
        status = pthread_barrier_wait(&barrier);
        if (status == PTHREAD_BARRIER_SERIAL_THREAD) {
            pthread_barrier_destroy(&barrier);
        } else if (status != 0) {
            printf("error wait barrier in thread with status = %d\n", status);
            exit(5);
        }
    }
    pthread_mutex_unlock(&mutex);
    pi *= 4;
    threadParams->result = pi;
    printf("%d thread make %ld  = %.10f\n", threadParams->startIndex / TASK_BLOCK_SIZE,threadIterCount, pi);
    pthread_exit(NULL);
}

void signalHandler(int signal){
    exit_flag = 1;
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

    signal(SIGINT, signalHandler);

    pthread_mutex_init(&mutex, NULL);
    int status = pthread_barrier_init(&barrier, NULL, pthreadCount);
    if (status != 0) {
        printf("main error: can't init barrier, status = %d\n", status);
        return 5;
    }

    pthread_t * threads = (pthread_t *)malloc(pthreadCount * sizeof(pthread_t));
    threadArgument_t * numStepsForThread = (threadArgument_t *)calloc(pthreadCount, sizeof(threadArgument_t));
    int error = 0;
    for (int i = 0; i < pthreadCount; ++i){
        numStepsForThread[i].startIndex = i * TASK_BLOCK_SIZE;
        numStepsForThread[i].stepSize = TASK_BLOCK_SIZE * pthreadCount;
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
    pthread_mutex_destroy(&mutex);
    free(threads);
    free(numStepsForThread);
    return 0;
}
