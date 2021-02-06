#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_COUNT_WIDGET 15

sem_t sem_a;
sem_t sem_b;
sem_t sem_c;

sem_t sem_widget;
sem_t sem_module;

int count_a = 0;
int count_b = 0;
int count_c = 0;

int count_module = 0;
int count_widget = 0;

pthread_t a;
pthread_t b;
pthread_t c;
pthread_t m;
pthread_t w;

void * producer_module() {
	while(1) {
	 	sem_wait(&sem_a);
		sem_wait(&sem_b);
		sem_post(&sem_module);
	}
}


void * producer_widget() {
	while(1) {
		sem_wait(&sem_c);
		sem_wait(&sem_module);
		sem_post(&sem_widget);
	}

}

void * producer_a() {
	while(1) {
		sleep(1);
		sem_post(&sem_a);
	}
}

void * producer_b() {
	while(1) {
		sleep(2);
		sem_post(&sem_b);
	}
}

void * producer_c() {
	while(1) {
		sleep(3);
		sem_post(&sem_c);
	}
}

void init_sem(){
    sem_init(&sem_a, 0, 0);
	sem_init(&sem_b, 0, 0);
	sem_init(&sem_c, 0, 0);
	sem_init(&sem_widget, 0, 0);
	sem_init(&sem_module, 0, 0);
}

void create_pthread(){
    pthread_create(&a, NULL, producer_a, NULL);
	pthread_create(&b, NULL, producer_b, NULL);
	pthread_create(&c, NULL, producer_c, NULL);
	pthread_create(&w, NULL, producer_widget, NULL);
	pthread_create(&m, NULL,  producer_module, NULL);
}

void get_value(){
    sem_getvalue(&sem_a, &count_a);
	sem_getvalue(&sem_b, &count_b);
	sem_getvalue(&sem_c, &count_c);
	sem_getvalue(&sem_widget, &count_widget);
	sem_getvalue(&sem_module, &count_module);
}

void cancel_pthread(){
    pthread_cancel(a);
	pthread_cancel(b);
	pthread_cancel(c);
	pthread_cancel(w);
	pthread_cancel(m);
}

void print_result(){
	printf("A: %d\n", count_a);
	printf("B: %d\n", count_b);
	printf("C: %d\n"), count_c;
	printf("Module: %d\n", count_module);
	printf("Wiget: %d\n", count_widget);
}

void destroy_sem(){
    sem_destroy(&sem_a);
	sem_destroy(&sem_b);
	sem_destroy(&sem_c);
	sem_destroy(&sem_widget);
	sem_destroy(&sem_module);
}


int main () {
	init_sem();
	create_pthread();

	while(count_widget < MAX_COUNT_WIDGET) {
        get_value();
        print_result();
	}

	cancel_pthread();
	destroy_sem();
	
	return 0;
}

