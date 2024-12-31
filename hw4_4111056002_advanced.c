#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
sem_t sem;
// params
#define BUFFER_SIZE 10
#define CONSUMER 5
#define PRODUCER 5
#define DATA 10000
#define str_len 10000
#define inf 256

sem_t mutex, full, empty;
int buffer[BUFFER_SIZE];
double burst_time_c[CONSUMER];
double burst_time_p[PRODUCER];
double turnaround_time_c[CONSUMER];
double turnaround_time_p[PRODUCER];

int in = 0;
int out = 0;

typedef struct{
	bool array[DATA + 1];
	int size;
} Data;

Data hashmap[CONSUMER];

bool* get(int key){
	return hashmap[key].array;
}

void put(int key, int val){
	hashmap[key].array[val] = true;
}

void create_out_folder(){
	struct stat st = {0};
	if (stat("out", &st) == 0){
		system("rm -rf out");
	}	
	mkdir("out", 0755);
}

void write_file(
		const char* file_name,
	       	const char* input_str,
	       	const char* mode,
		bool out
		){
	char file_path[inf];
	if(out) 
		sprintf(file_path, "out/%s", file_name);
	else
		sprintf(file_path, "%s", file_name);
	FILE *file = fopen(file_path, mode);
	fprintf(file, "%s", input_str);
	fclose(file);

}
double cal_diff(struct timespec start, struct timespec end){
	return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}
void *producer_func(void* arguAddr){
	struct timespec g_start;
	clock_gettime(CLOCK_MONOTONIC, &g_start);
	int n = *(int*) arguAddr;
	for(int i = 0; i < DATA / PRODUCER; ++i){
		sem_wait(&empty);
		sem_wait(&mutex);

		struct timespec start, end;
		clock_gettime(CLOCK_MONOTONIC, &start);

		buffer[in] = i + n * DATA / PRODUCER;
		int item = buffer[in];
		printf("Producer %d wrote %d\n", n, item);
		fflush(stdout);
		in = (in + 1) % BUFFER_SIZE;
		
		clock_gettime(CLOCK_MONOTONIC, &end);

		burst_time_p[n] += cal_diff(start, end);
		
		sem_post(&mutex);
		sem_post(&full);
	}
	struct timespec g_end;
	clock_gettime(CLOCK_MONOTONIC, &g_end);
	turnaround_time_p[n] = cal_diff(g_start, g_end);

	pthread_exit(NULL);
}

void *consumer_func(void* arguAddr){
	struct timespec g_start;
	clock_gettime(CLOCK_MONOTONIC, &g_start);
	int n = *(int*) arguAddr;
	for(int i = 0; i < DATA / CONSUMER; ++i){
		sem_wait(&full);
		sem_wait(&mutex);
		
		struct timespec start, end;
		clock_gettime(CLOCK_MONOTONIC, &start);

		int item = buffer[out];
		char file_name[inf];
		char file_content[inf];
		sprintf(file_name, "%d.txt", item);
		sprintf(file_content, "%d\n", n);
		printf("Consumer %d got %d\n", n, item);	
		fflush(stdout);
		put(n, item);
		write_file(file_name, file_content, "w", true);
		out = (out + 1) % BUFFER_SIZE;
		
		clock_gettime(CLOCK_MONOTONIC, &end);
		burst_time_c[n] += cal_diff(start, end) ;

		sem_post(&mutex);
		sem_post(&empty);
	}
	struct timespec g_end;
	clock_gettime(CLOCK_MONOTONIC, &g_end);
	turnaround_time_c[n] = cal_diff(g_start, g_end);
	pthread_exit(NULL);
}

int main(){

	sem_init(&mutex, 0, 1);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, BUFFER_SIZE);
	

	pthread_t p[PRODUCER];
	int index_p[PRODUCER];
	int index_c[CONSUMER];
	
	create_out_folder();
	for(int i = 0; i < PRODUCER; ++i){

		index_p[i] = i;
		pthread_create(p + i, NULL, producer_func, (void*) (index_p + i));
	}

	pthread_t c[CONSUMER];
	for(int i = 0; i < CONSUMER; ++i){
		
		index_c[i] = i;
		pthread_create(c + i, NULL, consumer_func, (void*) (index_c + i));
	}
	
	for(int i = 0; i < PRODUCER; ++i){ 
		pthread_join(p[i], NULL);
	}
	for(int i = 0; i < CONSUMER; ++i){ 
		pthread_join(c[i], NULL);
	}
	for(int i = 0; i < PRODUCER; ++i){
		double tt = turnaround_time_p[i];
		double wt = tt - burst_time_p[i];
		double bt = burst_time_p[i];
		printf("producer :%d\nturnaround time %.2e; waiting time %.2e; burst time %.2e;\n", i, tt, wt, bt); 
	}
	printf("------------------------\n");
	for(int i = 0; i < CONSUMER; ++i){
		double tt = turnaround_time_c[i];
		double wt = tt - burst_time_c[i];
		double bt = burst_time_c[i];
		printf("consumer :%d\ntunraround time %.2e; waiting time %.2e; burst time %.2e;\n", i, tt, wt, bt);
	}
	write_file("result.txt", "", "w", false);
	for(int i = 0; i < CONSUMER; ++i){
		bool* array = get(i);
		char buffer[str_len];
		sprintf(buffer, "---------------\nconsumer %d:\n", i);
		for(int j = 0; j < DATA; ++j){
			if(array[j]){
				char int_str[inf];
				snprintf(int_str, sizeof(int_str), "%d\n", j);
				strcat(buffer,  int_str);
			}
		}
		
		write_file("result.txt", buffer, "a", false);
	}
	sem_destroy(&mutex);
	sem_destroy(&full);
	sem_destroy(&empty);
	
	return 0;
}
