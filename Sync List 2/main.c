#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MAX_SRING_SIZE 80

typedef struct listNode{
    char * str;
    pthread_mutex_t mutex;
    struct listNode * next;
} node_t;

node_t * addStringNode(node_t * head, char * string){
    node_t * curHead = head;
    size_t strSize = strlen(string);
    if (strSize > MAX_SRING_SIZE){
        curHead = addStringNode(curHead, string + MAX_SRING_SIZE);
    }
    node_t * newHead = (node_t *)malloc(sizeof(node_t));
    pthread_mutex_init(&(newHead->mutex), NULL);
    pthread_mutex_lock(&(newHead->mutex));
    newHead->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));
    strncpy(newHead->str, string, strSize % (MAX_SRING_SIZE + 1));
    newHead->next = curHead;
    pthread_mutex_unlock(&(newHead->mutex));
    return newHead;
}

void printList(node_t * head){
    node_t * cur = head;
    node_t * prev;
    while (cur != NULL){
        pthread_mutex_lock(&(cur->mutex));
        printf("%s", cur->str);
        prev = cur;
        cur = cur->next;
        pthread_mutex_unlock(&(prev->mutex));
    }
}


void sort(node_t * head){
    if (head == NULL){
        return;
    }
    pthread_mutex_lock(&(head->mutex));
    node_t * left = head;                 
    node_t * right = head->next;  
    node_t * nextLeft = left->next;
    node_t * prevLeft = left;
    node_t * prevRight = right;   
    pthread_mutex_unlock(&(head->mutex));    
    node_t * temp = (node_t *)malloc(sizeof(node_t));     
    temp->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));  
    pthread_mutex_init(&(temp->mutex), NULL);     
    while (nextLeft != NULL){      
        pthread_mutex_lock(&(left->mutex));          
        while (right != NULL){
            pthread_mutex_lock(&(right->mutex));
            pthread_mutex_lock(&(temp->mutex));    // can be deleted?
            if (strncmp(left->str, right->str, MAX_SRING_SIZE) > 0){  
                strncpy(temp->str, left->str, MAX_SRING_SIZE);
                strncpy(left->str, right->str, MAX_SRING_SIZE);
                strncpy(right->str, temp->str, MAX_SRING_SIZE);
            }
            pthread_mutex_unlock(&(temp->mutex));  // can be deleted?
            prevRight = right;
            right = right->next; 
            pthread_mutex_unlock(&(prevRight->mutex));                   
        }
        prevLeft = left;
        left = left->next; 
        pthread_mutex_lock(&(left->mutex));                             
        right = left->next; 
        nextLeft = left->next; 
        pthread_mutex_unlock(&(left->mutex));
        pthread_mutex_unlock(&(prevLeft->mutex));              
    }
    pthread_mutex_destroy(&(temp->mutex));
    free(temp->str);
    free(temp);
}


node_t * list;

pthread_mutex_t listMutex;
int sortFlag = 0;


void setSortFlag(int signo){
    sortFlag = 1;
    signal(SIGALRM, setSortFlag);
    alarm(5);
}

void * sortThread(void * param){
    while (1){
        if (sortFlag){
            sort(list);
            sortFlag = 0;
        }
    }
    
}

int main(int argc, char ** argv){
    signal(SIGALRM, setSortFlag);
    pthread_t thread;
    int error = pthread_create(&thread, NULL, sortThread, NULL);
    if (error){
        strerror(error);
        return 1;
    }
    alarm(5);
    char userString[MAX_SRING_SIZE + 1] = {0};
    while (1){
        fgets(userString, MAX_SRING_SIZE, stdin);
        if (!strcmp("\n", userString)){
            printf("LIST PRINTING--------------------------\n");
            printList(list);
            printf("---------------------------------------\n");
        }
        else{
            list = addStringNode(list, userString);
        }
    }
}