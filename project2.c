#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

// global variables
int number_of_threads; 
int steps;
int sleep_time;

// create mutex
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// info about a thread
typedef struct thread_info { 
    int id;        // thread id
    int direction; // direction: 0 for down, 1 for up
} thread_info_t;

// enter_stairs: acquire mutex before entering stairs
void enter_stairs(int dir, int id) {
    pthread_mutex_lock(&m);
    printf("Thread %d entering stairs (direction: %s)\n", id, dir == 1 ? "up" : "down");
}

// exit_stairs: release mutex after exiting stairs
void exit_stairs(int dir, int id) {
    printf("Thread %d exiting stairs (direction: %s)\n", id, dir == 1 ? "up" : "down");
    pthread_mutex_unlock(&m);
}

void * thread_function(void *vinfo) {
    thread_info_t* info = (thread_info_t *)vinfo;
    
    // enter stairs (acquire mutex)
    enter_stairs(info->direction, info->id);
    
    // simulate walking on stairs
    sleep(sleep_time);
    
    // exit stairs (release mutex)
    exit_stairs(info->direction, info->id);
    
    pthread_exit(NULL);
}

int main(){
    // initialize random seed
    srand(time(NULL));
    
    number_of_threads = rand() % 30 + 1;  // random between 1 and 30
    steps = rand() % 13 + 1;              // random between 1 and 13
    sleep_time = steps;
    
    // create thread info
    
    
    // create multiple threads
    
    
    // wait for all threads to finish
    
    
    // clean up resources
    free(info);
    pthread_mutex_destroy(&m);
    
    return 0;
}