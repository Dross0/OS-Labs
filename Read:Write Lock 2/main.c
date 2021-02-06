#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


#define MAX_SRING_SIZE 80
#define ERROR_CREATE_THREAD  -1

int sortFlag = 0;


typedef struct listNode{
    char * str;
    pthread_rwlock_t rwlock;
    struct listNode * next;
} node_t;

node_t * list;

node_t * createNode(node_t * curHead, char * string){
    node_t * node = (node_t *)malloc(sizeof(node_t));
    size_t strSize = strlen(string);
    pthread_rwlock_init(&(node->rwlock), NULL);
    node->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));
    strncpy(node->str, string, strSize);
    node->next = curHead;
    return node;
}

void swapStrings(node_t * node1, node_t * node2){
    node_t * temp = (node_t *)malloc(sizeof(node_t));
    temp->str = (char *)calloc(MAX_SRING_SIZE + 1, sizeof(char));
    strncpy(temp->str, node1->str, MAX_SRING_SIZE);
    strncpy(node1->str, node2->str, MAX_SRING_SIZE);
    strncpy(node2->str, temp->str, MAX_SRING_SIZE);
    free(temp->str);
    free(temp);
}

node_t * addStringNode(node_t * head, char * string){
    node_t * curHead = head;
    node_t * newHead = createNode(curHead, string);
    return newHead;
}

void printList(node_t * head){
    node_t * cur = head;
    node_t * prev;
    while (cur != NULL){
        printf("%s", cur->str);
        prev = cur;
        cur = cur->next;
    }
}


void sort(node_t * head){
    if (head == NULL){
        return;
    }
    pthread_rwlock_wrlock(&(head->rwlock));
    node_t * left = head;
    node_t * right = head->next;
    node_t * nextLeft = left->next;
    node_t * prevLeft = left;
    node_t * prevRight = right;
    pthread_rwlock_unlock(&(head->rwlock));
    int flag = 0;
    while (nextLeft != NULL){
        
        while (right != NULL){
	    pthread_rwlock_wrlock(&(left->rwlock));
	    pthread_rwlock_wrlock(&(right->rwlock));
            if (strncmp(left->str, right->str, MAX_SRING_SIZE) > 0){
                swapStrings(left, right);
                flag = 1;
                sleep(1);
            }
            prevRight = right;
            right = right->next;
	    pthread_rwlock_unlock(&(left->rwlock));
	    pthread_rwlock_unlock(&(prevRight->rwlock));	    
        }
        if (!flag){
            return;
        }
        flag = 0;
        prevLeft = left;
        left = left->next;
        right = left->next;
        nextLeft = left->next;                               
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
            sort(list);
            sortFlag = 0;
        }
    }

}

int main(int argc, char ** argv){
    signal(SIGALRM, setSortFlag);
    pthread_t thread;
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
        if (!strcmp("\n", userString)){
            printf("LIST PRINTING-------------\n");
            printList(list);
            printf("-------------------- -----\n");
        }
        else{
            list = addStringNode(list, userString);
        }
    }
}



