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
#include <unistd.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/wait.h> 

unsigned num_processes; // number of worker processes
unsigned integration_divisions; // number of divisions at integration

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

/**
 * Global Variables
 */ 

// Identifier in shmid 
int shmid;
// Shared memory segment.
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
	double interval_size = 1.0 / integration_divisions; // The circle radius is 1.0

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
	pid_t child_pid, wpid;
	int status = 0;
	task *tasks;
	

	// Argument parsing
	if (argc != 3 || sscanf(argv[1], "%u", &num_processes) != 1 || sscanf(argv[2], "%u", &integration_divisions) != 1) {
		printf("usage: %s <num_processos> <num_pontos>\n", argv[0]);
		return 1;
	}

	/**
	 * ICP 
	 */
	// ftok to generate unique key 
	key_t key = ftok("shmfile", 65); 

	// shmget returns an identifier in shmid
    if( (shmid = shmget(key, 1024, 0666 | IPC_CREAT) ) == -1 ) {
		DIE("Unable to obtain identifier in shmid.\n");
	}

	// shmat to attach to shared memory 
    if( (segment = (shared_memory*) shmat(shmid, (void*) 0 , 0)) == (shared_memory*) -1 ) { // Cast to avoid warning
		DIE("Unable to attach shared memory.\n");
	}

	// Initialize mutex with default attributes
	if(pthread_mutex_init(&segment->lock, NULL))
		DIE("Failed to init mutex.\n");

	// Tasks' arrays
	if((tasks = malloc(num_processes * sizeof(task))) == NULL)
		DIE("Tasks malloc failed.\n");


	// Initialize processes with default attributes.
	// The work is being splitted as evenly as possible between threads.
	int processes_with_one_more_work = integration_divisions % num_processes;
	for (int i = 0; i < num_processes; ++i) {
		int work_size = integration_divisions / num_processes;
		if (i < processes_with_one_more_work)
			work_size += 1;
		tasks[i].start = i * work_size;
		tasks[i].end = (i + 1) * work_size;

		// Fork a child
		child_pid = fork();

		// Child Process
		if( child_pid == 0 ) { 
			process_work( (void *)&tasks[i] );
			// Child processes do not need to continue to process the rest of the code.
			exit(EXIT_SUCCESS); 
		} 
		// Parent Process
		else if ( child_pid != -1 ) {
			// Parent is Waiting
			// DO NOTHING
		}
		else {
			DIE("Error while calling the fork function.\n");
		}
	}

	// Parent process waits for all children processes to finish.
	while ((wpid = wait(&status)) > 0);

	printf("%.12f\n", segment->pi_by_4 * 4); // "pi ~= %.12f\n"
	// 3.141592653589
	
	if(pthread_mutex_destroy(&segment->lock)) // Destroy mutex
		DIE("Failed to destroy mutex.\n");

	//detach from shared memory  
    shmdt(segment); 
    
    // destroy the shared memory 
    shmctl(shmid,IPC_RMID,NULL); 

	free(tasks);
	return 0;
}
