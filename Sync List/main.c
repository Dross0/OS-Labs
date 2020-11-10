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
int sortFlag = 0;


void setSortFlag(int signo){
    sortFlag = 1;
    signal(SIGALRM, setSortFlag);
    alarm(5);
}

void * sortThread(void * param){
    while (1){
        if (sortFlag){
            pthread_mutex_lock(&listMutex);
            sort(list);
            sortFlag = 0;
            pthread_mutex_unlock(&listMutex);
        }
    }
    
}

int main(int argc, char ** argv){
    signal(SIGALRM, setSortFlag);
    pthread_t thread;
    pthread_mutex_init(&listMutex, NULL);
    pthread_mutex_init(&sortFlagMutex, NULL);
    int error = pthread_create(&thread, NULL, sortThread, NULL);
    if (error){
        strerror(error);
        return 1;
    }
    alarm(5);
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