#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>



#define MAX_SRING_SIZE 80
#define ERROR_CREATE_THREAD  -1

pthread_rwlock_t rwlock;
int sortFlag = 0;


typedef struct listNode{
    char * str;
    struct listNode * next;
} node_t;

node_t * list;

node_t * createNode(){
    node_t * node = (node_t *)malloc(sizeof(node_t));
    node->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));
    return node;
}

void freeNode(node_t * node){
    if (node == NULL){
        return;
    }
    free(node->str);
    free(node);
}

void swapStrings(node_t * node1, node_t * node2){
    if (node1 == NULL || node2 == NULL){
        return;
    }
    node_t * temp = createNode();
    strncpy(temp->str, node1->str, MAX_SRING_SIZE);
    strncpy(node1->str, node2->str, MAX_SRING_SIZE);
    strncpy(node2->str, temp->str, MAX_SRING_SIZE);
    freeNode(temp);

}

node_t * addStringNode(node_t * head, char * string){
    node_t * curHead = head;
    size_t strSize = strlen(string);
    node_t * newHead = createNode();
    strncpy(newHead->str, string, strSize);
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
    int flag = 0;    
    while (left->next != NULL){                
            while (right != NULL){  
                if (strncmp(left->str, right->str, MAX_SRING_SIZE) > 0){  
                    swapStrings(left, right);
			        flag = 1;
                }
                right = right->next;                    
            }
	    if (!flag){
		    return;
	    }
	    flag = 0;
        left = left->next;                              
        right = left->next;                         
    }
}



void setSortFlag(int signo){
    sortFlag = 1;
    signal(SIGALRM, setSortFlag);
    alarm(5);
}

void * sortThread(void * param){
    while (1){
        if (sortFlag){
            pthread_rwlock_wrlock(&rwlock);
            sort(list);
            sortFlag = 0;
            pthread_rwlock_unlock(&rwlock);
        }
    }
    
}

int main(int argc, char ** argv){
    signal(SIGALRM, setSortFlag);
    pthread_t thread;
    pthread_rwlock_init(&rwlock, NULL);
    
    int status = pthread_create(&thread, NULL, sortThread, NULL);
    if (status != 0)
    {
        printf("error: can't create thread, status = %d\n", status);
        exit(ERROR_CREATE_THREAD);
    }
    alarm(5);
    char userString[MAX_SRING_SIZE + 1] = {0};
    while (1){
        fgets(userString, MAX_SRING_SIZE, stdin);
        pthread_rwlock_wrlock(&rwlock);
        if (!strcmp("\n", userString)){
            printf("LIST PRINTING-------------\n");
            printList(list);
            printf("-------------------- -----\n");
        }
        else{
            list = addStringNode(list, userString);
        }
        pthread_rwlock_unlock(&rwlock);
    }
}
