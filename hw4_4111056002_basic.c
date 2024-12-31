#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem;
// params
#define BUFFER_SIZE 10
#define CONSUMER 5
#define PRODUCER 5
#define DATA 20

sem_t mutex, full, empty;
int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;


void *producer_func(void* arguAddr){
	int n = *(int*) arguAddr;
	for(int i = 0; i < DATA / PRODUCER; ++i){
		sem_wait(&empty);
		sem_wait(&mutex);
		
		buffer[in] = i + n * DATA / PRODUCER;
		int item = buffer[in];
		printf("Producer %d wrote %d\n", n, item);
		fflush(stdout);
		in = (in + 1) % BUFFER_SIZE;

		sem_post(&mutex);
		sem_post(&full);
	}
	pthread_exit(NULL);
}

void *consumer_func(void* arguAddr){
	int n = *(int*) arguAddr;
	for(int i = 0; i < DATA / CONSUMER; ++i){
		sem_wait(&full);
		sem_wait(&mutex);
		
		int item = buffer[out];
		printf("Consumer %d got %d\n", n, item);
		fflush(stdout);
		out = (out + 1) % BUFFER_SIZE;

		sem_post(&mutex);
		sem_post(&empty);
	}
	pthread_exit(NULL);
}

int main(){
	sem_init(&mutex, 0, 1);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, BUFFER_SIZE);
	

	pthread_t p[PRODUCER];
	int index_p[PRODUCER];
	int index_c[CONSUMER];
	for(int i = 0; i < PRODUCER; ++i){
		index_p[i] = i;
		pthread_create(p + i, NULL, producer_func, (void*) (index_p + i));
	}

	pthread_t c[CONSUMER];
	for(int i = 0; i < CONSUMER; ++i){
		index_c[i] = i;
		pthread_create(c + i, NULL, consumer_func, (void*) (index_c + i));
	}

	for(int i = 0; i < PRODUCER; ++i) pthread_join(p[i], NULL);
	for(int i = 0; i < CONSUMER; ++i) pthread_join(c[i], NULL);

	sem_destroy(&mutex);
	sem_destroy(&full);
	sem_destroy(&empty);
	
	return 0;
}
