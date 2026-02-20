#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

// global variables
int number_of_threads; 
int steps;
int sleep_time;

// create mutex
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_up = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_down = PTHREAD_COND_INITIALIZER;

int occupancy = 0;
int cur_dir = -1; 
int waiting[2] = {0, 0};
int turn = -1;
int b_cnt = 0;
int b_max = 0;

// info about a thread
typedef struct thread_info { 
    int id;        // thread id
    int direction; // direction: 0 for down, 1 for up
    struct timespec start; // start time
    struct timespec end;   // end time
    double turnaround;     // total turnaround time
} thread_info_t;

static double get_dt(struct timespec s, struct timespec e) {
    double ds = e.tv_sec - s.tv_sec;
    double dns = e.tv_nsec - s.tv_nsec;
    return ds + dns / 1e9;
}

int is_blocked(int d) {
    if (occupancy > 0 && cur_dir != d) return 1;
    if (occupancy == 0 && turn != -1 && turn != d && waiting[turn] > 0) return 1;
    if (occupancy > 0 && cur_dir == d) {
        if (waiting[1 - d] > 0 && b_cnt >= b_max) return 1;
    }
    return 0;
}

// enter_stairs: acquire mutex before entering stairs
void enter_stairs(int dir, int id) {
    pthread_mutex_lock(&m);
    
    waiting[dir]++;
    while (is_blocked(dir) || occupancy >= steps) {
        if (dir == 1) pthread_cond_wait(&c_up, &m);
        else pthread_cond_wait(&c_down, &m);
    }

    if (occupancy == 0) {
        cur_dir = dir;
        b_cnt = 0;
    }

    waiting[dir]--;
    occupancy++;
    b_cnt++;

    printf("%d thread is entering stairs at direction: %s [%d/%d]\n", id, dir == 1 ? "up" : "down", occupancy, steps);
    pthread_mutex_unlock(&m);
}

// exit_stairs: release mutex after exiting stairs
void exit_stairs(int dir, int id) {
    pthread_mutex_lock(&m);
    
    occupancy--;
    printf("%d thread is exiting stairs at direction: %s [%d/%d]\n", id, dir == 1 ? "up" : "down", occupancy, steps);

    if (dir == 1) pthread_cond_broadcast(&c_up);
    else pthread_cond_broadcast(&c_down);

    if (occupancy == 0) {
        cur_dir = -1;
        int other = 1 - dir;

        if (waiting[other] > 0) turn = other;
        else if (waiting[dir] > 0) turn = dir;
        else turn = -1;

        b_cnt = 0;
        pthread_cond_broadcast(&c_up);
        pthread_cond_broadcast(&c_down);
    }

    pthread_mutex_unlock(&m);
}

void * thread_function(void *vinfo) {
    thread_info_t* info = (thread_info_t *)vinfo;
    
    clock_gettime(CLOCK_MONOTONIC, &info->start);
    
    // enter stairs (acquire mutex)
    enter_stairs(info->direction, info->id);
    
    // simulate walking on stairs
    sleep(sleep_time);
    
    // exit stairs (release mutex)
    exit_stairs(info->direction, info->id);
    
    clock_gettime(CLOCK_MONOTONIC, &info->end);
    info->turnaround = get_dt(info->start, info->end);
    
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    // initialize random seed
    srand(time(NULL));
    
    if (argc == 3) {
        number_of_threads = atoi(argv[1]);
        steps = atoi(argv[2]);
    } else {
        number_of_threads = rand() % 30 + 1;  // random between 1 and 30
        steps = rand() % 13 + 1;              // random between 1 and 13
    }
    
    sleep_time = steps;
    b_max = steps;
    
    // create thread info
    pthread_t *tids = malloc(sizeof(pthread_t) * number_of_threads);
    thread_info_t *info = malloc(sizeof(thread_info_t) * number_of_threads);
    
    // create multiple threads
    for (int i = 0; i < number_of_threads; i++) {
        info[i].id = i;
        info[i].direction = rand() % 2;
        pthread_create(&tids[i], NULL, thread_function, &info[i]);
    }
    
    // wait for all threads to finish
    double total_t = 0;
    for (int i = 0; i < number_of_threads; i++) {
        pthread_join(tids[i], NULL);
        printf("thread %d turnaround is %.4fs\n", info[i].id, info[i].turnaround);
        total_t += info[i].turnaround;
    }

    printf("\naverage is %.3fs\n", total_t / number_of_threads);
    
    // clean up resources
    free(tids);
    free(info);  // TODO
    pthread_mutex_destroy(&m);
    
    return 0;
}