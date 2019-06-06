/*
*      pi.c
*      Approximates PI integrating the area of a quarter of circle with
*      Riemann integration.
*
*      by Lucas Emanuel Ramos Fernandes Koontz (nUSP 11356241)
*      (adapted from Matheus Tavares' and Giuliano Belinassi's work)
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

// ICP Libraries
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <unistd.h> 

unsigned num_processos; // number of worker threads
unsigned N; // number of divisions at integration

// Defines a task of integration over the quarter of circle, with an x-axis
// start and end indexes. The work must be done in the interval [start, end[
typedef struct task {
	int start, end;
} task;

// Defines a struct for shared memory
typedef struct shared_memory {
	double pi_by_4; // approximation of pi/4
	pthread_mutex_t lock; // protects pi_by_4 writes
} shared_memory;

// Global Variable with the references to the shared memory segment.
shared_memory *segment;

#define DIE(...) { \
	fprintf(stderr, __VA_ARGS__); \
	exit(EXIT_FAILURE); \
}

// This is the function to be executed by all worker_processes. It receives a task
// and sums the process's work at global pi_by_4 variable.
void *process_work(void *arg) {
	task *t = (task *) arg;
	double acc = 0; // Thread's local integration variable
	double interval_size = 1.0 / N; // The circle radius is 1.0

	// Integrates f(x) = sqrt(1 - x^2) in [t->start, t->end[
	for(int i = t->start; i < t->end; ++i) {
		double x = (i * interval_size) + interval_size / 2;
		acc += sqrt(1 - (x * x)) * interval_size;
	}

	// This is a critical section. As we are going to write to a global
	// value, the operation must me protected against race conditions.
	pthread_mutex_lock(&segment->lock);
	segment->pi_by_4 += acc;
	pthread_mutex_unlock(&segment->lock);

	return NULL;
}


int main(int argc, char **argv)
{
	pthread_t *threads;
	task *tasks;

	// Argument parsing
	if (argc != 3 || sscanf(argv[1], "%u", &num_processos) != 1 || sscanf(argv[2], "%u", &N) != 1) {
		printf("usage: %s <num_processos> <num_pontos>\n", argv[0]);
		return 1;
	}

	/**
	 * ICP 
	 */
	// ftok to generate unique key 
	key_t key = ftok("shmfile", 65); 

	// shmget returns an identifier in shmid 
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT); 

	// shmat to attach to shared memory 
    segment = (shared_memory*) shmat(shmid, (void*) 0 , 0); 

	// Initialize mutex with default attributes
	if(pthread_mutex_init(&segment->lock, NULL))
		DIE("Failed to init mutex\n");

	// Tasks' arrays
	if((threads = malloc(num_processos * sizeof(pthread_t))) == NULL)
		DIE("Threads malloc failed\n");
	if((tasks = malloc(num_processos * sizeof(task))) == NULL)
		DIE("Tasks malloc failed\n");


	// Initialize processes with default attributes.
	// The work is being splitted as evenly as possible between threads.
	int processes_with_one_more_work = N % num_processos;
	for (int i = 0; i < num_processos; ++i) {
		int work_size = N / num_processos;
		if (i < processes_with_one_more_work)
			work_size += 1;
		tasks[i].start = i * work_size;
		tasks[i].end = (i + 1) * work_size;

		if(pthread_create(&threads[i], NULL, process_work, (void *)&tasks[i]))
			DIE("Failed to create thread %d\n", i)
	}

	// Finish threads and ignore their return values
	for (int i = 0; i < num_processos; ++i) {
		if(pthread_join(threads[i], NULL))
				DIE("failed to join thread %d\n", i);
	}

	printf("pi ~= %.12f\n", segment->pi_by_4 * 4);

	if(pthread_mutex_destroy(&segment->lock)) // Destroy mutex
		DIE("Failed to destroy mutex\n");

	//detach from shared memory  
    shmdt(segment); 
    
    // destroy the shared memory 
    shmctl(shmid,IPC_RMID,NULL); 

	free(threads);
	free(tasks);
	return 0;
}
