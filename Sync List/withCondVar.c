#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MAX_SRING_SIZE 80

typedef struct listNode{
    char * str;
    struct listNode * next;
} node_t;

node_t * addStringNode(node_t * head, char * string){
    node_t * curHead = head;
    size_t strSize = strlen(string);
    if (strSize > MAX_SRING_SIZE){
        curHead = addStringNode(curHead, string + MAX_SRING_SIZE);
    }
    node_t * newHead = (node_t *)malloc(sizeof(node_t));
    newHead->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));
    strncpy(newHead->str, string, strSize % (MAX_SRING_SIZE + 1));
    newHead->next = curHead;
    return newHead;
}

void printList(node_t * head){
    node_t * cur = head;
    while (cur != NULL){
        printf("%s", cur->str);
        cur = cur->next;
    }
}


void sort(node_t * head){
    if (head == NULL){
        return;
    }
    node_t * left = head;                 
    node_t * right = head->next;          
    node_t * temp = (node_t *)malloc(sizeof(node_t));     
    temp->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));       
    while (left->next != NULL){                
            while (right != NULL){  
                    if (strncmp(left->str, right->str, MAX_SRING_SIZE) > 0){  
                        strncpy(temp->str, left->str, MAX_SRING_SIZE);
                        strncpy(left->str, right->str, MAX_SRING_SIZE);
                        strncpy(right->str, temp->str, MAX_SRING_SIZE);
                    }
                right = right->next;                    
            }
        left = left->next;                              
        right = left->next;                         
    }
    free(temp->str);
    free(temp);
}


node_t * list;

pthread_mutex_t listMutex;
pthread_mutex_t sortFlagMutex;
pthread_cond_t condVar;
int sortFlag = 0;


void * sortThreadFunc(void * param){
    while (1){
        pthread_mutex_lock(&sortFlagMutex);
        while (!sortFlag){
            pthread_cond_wait(&condVar, &sortFlagMutex);
        }
        pthread_mutex_lock(&listMutex);
        sort(list);
        pthread_mutex_unlock(&listMutex);
        sortFlag = 0;
        pthread_mutex_unlock(&sortFlagMutex);
    }
    
}

void * alarmThreadFunc(void * param){
    while (1){
        sleep(5);
        pthread_mutex_lock(&sortFlagMutex);
        sortFlag = 1;
        pthread_mutex_unlock(&sortFlagMutex);
        pthread_cond_signal(&condVar);
    }
}

int main(int argc, char ** argv){
    pthread_t sortThread;
    pthread_t alarmThread;
    pthread_mutex_init(&listMutex, NULL);
    pthread_mutex_init(&sortFlagMutex, NULL);
    pthread_cond_init(&condVar, NULL);
    int error = pthread_create(&sortThread, NULL, sortThreadFunc, NULL);
    if (error){
        strerror(error);
        return 1;
    }
    error = pthread_create(&alarmThread, NULL, alarmThreadFunc, NULL);
    if (error){
        strerror(error);
        return 1;
    }
    char userString[MAX_SRING_SIZE + 1] = {0};
    while (1){
        fgets(userString, MAX_SRING_SIZE, stdin);
        pthread_mutex_lock(&listMutex);
        if (!strcmp("\n", userString)){
            printf("LIST PRINTING--------------------------\n");
            printList(list);
            printf("---------------------------------------\n");
        }
        else{
            list = addStringNode(list, userString);
        }
        pthread_mutex_unlock(&listMutex);
    }
}