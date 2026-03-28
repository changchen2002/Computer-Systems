#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHANCE_OF_IO_REQUEST 10
#define CHANCE_OF_IO_COMPLETE 4
#define TIME_QUANTUM 3

typedef enum {
    JOB_NEW,
    JOB_READY,
    JOB_RUNNING,
    JOB_WAITING,
    JOB_FINISHED
} JobState;

typedef struct Job {
    int id;
    int arrival_time;
    int total_cpu_time;
    int remaining_time;
    int priority;
    int time_in_quantum;
    int first_run_time;
    int completion_time;
    JobState state;
    struct Job *next;
    int active_ticks;
    int wait_ticks;
    int sleep_ticks;
} Job;

typedef struct {
    Job *head;
    Job *tail;
    int length;
} JobQueue;

static int clock_time = 0;

static void os_srand(unsigned int seed)
{
    srand(seed);
}

static int os_rand(void)
{
    return rand();
}

int IO_request(void)
{
    if (os_rand() % CHANCE_OF_IO_REQUEST == 0) {
        return 1;
    }
    return 0;
}

int IO_complete(void)
{
    if (os_rand() % CHANCE_OF_IO_COMPLETE == 0) {
        return 1;
    }
    return 0;
}

static const char *job_state_name(JobState state)
{
    switch (state) {
    case JOB_NEW:      return "NEW";
    case JOB_READY:    return "READY";
    case JOB_RUNNING:  return "RUNNING";
    case JOB_WAITING:  return "WAITING";
    case JOB_FINISHED: return "FINISHED";
    default:           return "UNKNOWN";
    }
}

static void queue_init(JobQueue *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
}

static int queue_is_empty(const JobQueue *queue)
{
    return queue->head == NULL;
}

static void enqueue(JobQueue *queue, Job *job)
{
    job->next = NULL;
    if (queue->tail == NULL) {
        queue->head = job;
        queue->tail = job;
    } else {
        queue->tail->next = job;
        queue->tail = job;
    }
    queue->length++;
}

static Job *dequeue(JobQueue *queue)
{
    Job *job;
    if (queue->head == NULL) {
        return NULL;
    }
    job = queue->head;
    queue->head = job->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    job->next = NULL;
    queue->length--;
    return job;
}

static void log_transition(const Job *job, const char *event)
{
    printf("clock=%2d job=%d state=%-8s event=%s\n",
           clock_time, job->id, job_state_name(job->state), event);
}

static void move_to_ready(JobQueue *ready_queue, Job *job, const char *reason)
{
    job->state = JOB_READY;
    job->time_in_quantum = 0;
    enqueue(ready_queue, job);
    log_transition(job, reason);
}

static void move_to_waiting(JobQueue *waiting_queue, Job *job, const char *reason)
{
    job->state = JOB_WAITING;
    job->time_in_quantum = 0;
    enqueue(waiting_queue, job);
    log_transition(job, reason);
}

static void mark_finished(Job *job)
{
    job->state = JOB_FINISHED;
    job->completion_time = clock_time + 1;
    job->time_in_quantum = 0;
    log_transition(job, "completed");
}

static void add_new_incoming_jobs(Job jobs[], int job_count, JobQueue *ready_queue)
{
    int i;
    for (i = 0; i < job_count; i++) {
        if (jobs[i].state == JOB_NEW && jobs[i].arrival_time <= clock_time) {
            move_to_ready(ready_queue, &jobs[i], "arrived");
        }
    }
}

static void process_waiting_jobs(JobQueue *waiting_queue, JobQueue *ready_queue)
{
    Job *job;
    Job *next_job;
    Job *previous = NULL;

    job = waiting_queue->head;
    while (job != NULL) {
        next_job = job->next;
        if (IO_complete()) {
            if (previous == NULL) {
                waiting_queue->head = next_job;
            } else {
                previous->next = next_job;
            }
            if (waiting_queue->tail == job) {
                waiting_queue->tail = previous;
            }
            waiting_queue->length--;
            job->next = NULL;
            move_to_ready(ready_queue, job, "io-complete");
        } else {
            previous = job;
        }
        job = next_job;
    }
}

static Job *choose_job_to_execute(JobQueue *ready_queue)
{
    Job *job = dequeue(ready_queue);
    if (job != NULL) {
        job->state = JOB_RUNNING;
        if (job->first_run_time < 0) {
            job->first_run_time = clock_time;
        }
        log_transition(job, "dispatched");
    }
    return job;
}

static int jobs_remaining(Job jobs[], int job_count, const JobQueue *ready_queue,
                          const JobQueue *waiting_queue, const Job *current_job)
{
    int i;
    if (!queue_is_empty(ready_queue) || !queue_is_empty(waiting_queue) || current_job != NULL) {
        return 1;
    }
    for (i = 0; i < job_count; i++) {
        if (jobs[i].state != JOB_FINISHED) {
            return 1;
        }
    }
    return 0;
}

static void update_job_metrics(Job *job_list, int total_count)
{
    int idx;
    for (idx = 0; idx < total_count; idx++) {
        Job *j = &job_list[idx];
        if (j->state == JOB_READY) {
            j->wait_ticks++;
        } else if (j->state == JOB_WAITING) {
            j->sleep_ticks++;
        } else if (j->state == JOB_RUNNING) {
            j->active_ticks++;
        }
    }
}

static void reset_jobs(Job *jobs, int n_jobs)
{
    int i;
    for (i = 0; i < n_jobs; i++) {
        jobs[i].remaining_time  = jobs[i].total_cpu_time;
        jobs[i].state           = JOB_NEW;
        jobs[i].time_in_quantum = 0;
        jobs[i].first_run_time  = -1;
        jobs[i].completion_time = -1;
        jobs[i].wait_ticks      = 0;
        jobs[i].sleep_ticks     = 0;
        jobs[i].active_ticks    = 0;
        jobs[i].next            = NULL;
    }
}

static void sort_jobs_by_arrival(Job *jobs, int n)
{
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (jobs[i].arrival_time > jobs[j].arrival_time ||
               (jobs[i].arrival_time == jobs[j].arrival_time &&
                jobs[i].id > jobs[j].id)) {
                Job tmp  = jobs[i];
                jobs[i]  = jobs[j];
                jobs[j]  = tmp;
            }
        }
    }
}

static void display_scheduler_report(Job *work_list, int n_jobs, const char *scheduler_name)
{
    int i;
    int max_time = 0;
    int min_turnaround = -1, max_turnaround = -1;
    double sum_turnaround = 0, sum_wait = 0, sum_sleep = 0;

    for (i = 0; i < n_jobs; i++) {
        int turnaround = work_list[i].completion_time - work_list[i].arrival_time;
        if (work_list[i].completion_time > max_time)
            max_time = work_list[i].completion_time;
        if (min_turnaround < 0 || turnaround < min_turnaround) min_turnaround = turnaround;
        if (turnaround > max_turnaround) max_turnaround = turnaround;
        sum_turnaround += turnaround;
        sum_wait       += work_list[i].wait_ticks;
        sum_sleep      += work_list[i].sleep_ticks;
    }

    printf("\n %s scheduler \n\n", scheduler_name);

    printf("     Job# | Total time       | Total time       | Total time\n");
    printf("          | in ready to run  | in sleeping on   | in system\n");
    printf("          | state            | I/O state        |\n");
    printf("==========+==================+==================+==========\n");

    for (i = 0; i < n_jobs; i++) {
        int in_system = work_list[i].completion_time - work_list[i].arrival_time;
        printf("pid%-6d| %-17d| %-17d| %d\n",
            work_list[i].id,
            work_list[i].wait_ticks,
            work_list[i].sleep_ticks,
            in_system);
    }

    printf("==========+==================+==================+==========\n");
    printf("Total simulation run time: %d\n",             max_time);
    printf("Total number of jobs: %d\n",                  n_jobs);
    printf("Shortest job completion time: %d\n",          min_turnaround);
    printf("Longest job completion time: %d\n",           max_turnaround);
    printf("Average job completion time: %d\n",   (int)(sum_turnaround / n_jobs));
    printf("Average time in ready queue: %d\n",   (int)(sum_wait       / n_jobs));
    printf("Average time sleeping on I/O state: %d\n", (int)(sum_sleep / n_jobs));
}

static void run_scheduler(Job jobs[], int job_count)
{
    JobQueue ready_q, waiting_q;
    Job *active_job = NULL;

    queue_init(&ready_q);
    queue_init(&waiting_q);
    clock_time = 0;
    os_srand(1);

    add_new_incoming_jobs(jobs, job_count, &ready_q);

    while (jobs_remaining(jobs, job_count, &ready_q, &waiting_q, active_job)) {
        if (!active_job) active_job = choose_job_to_execute(&ready_q);

        add_new_incoming_jobs(jobs, job_count, &ready_q);
        process_waiting_jobs(&waiting_q, &ready_q);

        if (!active_job) {
            printf("clock=%2d cpu=IDLE\n", clock_time++);
            continue;
        }

        update_job_metrics(jobs, job_count);

        active_job->remaining_time--;
        active_job->time_in_quantum++;

        printf("clock=%2d job=%d state=%-8s event=cpu-burst remaining=%d\n",
               clock_time, active_job->id, job_state_name(active_job->state),
               active_job->remaining_time);

        if (active_job->remaining_time == 0) {
            mark_finished(active_job);
            active_job = NULL;
        } else if (IO_request()) {
            move_to_waiting(&waiting_q, active_job, "io-request");
            active_job = NULL;
        } else if (active_job->time_in_quantum >= TIME_QUANTUM) {
            move_to_ready(&ready_q, active_job, "time-slice-expired");
            active_job = NULL;
        }

        clock_time++;
    }
}

static int load_jobs_from_input(Job **out_ptr)
{
    char buffer[256];
    int n = 0, size = 16;
    Job *list = malloc(sizeof(Job) * size);

    if (!list) return 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        int p, a, s, pr;
        if (buffer[0] == '\n' || buffer[0] == '#' || buffer[0] == '\r') continue;

        if (sscanf(buffer, "%d:%d:%d:%d", &p, &a, &s, &pr) == 4) {
            if (n >= size) {
                size *= 2;
                list = realloc(list, sizeof(Job) * size);
                if (!list) return 0;
            }
            list[n].id              = p;
            list[n].priority        = pr;
            list[n].next            = NULL;

            list[n].arrival_time    = a;
            list[n].total_cpu_time  = s;
            list[n].remaining_time  = s; 

            list[n].state           = JOB_NEW;
            list[n].time_in_quantum = 0;

            list[n].first_run_time  = -1;
            list[n].completion_time = -1;
            list[n].wait_ticks      = 0;
            list[n].sleep_ticks     = 0;
            list[n].active_ticks    = 0;

            n++;
        }
    }

    *out_ptr = list;
    return n;
}

int main(void)
{
    Job *job_array = NULL;
    int n = load_jobs_from_input(&job_array);

    if (n <= 0) {
        fprintf(stderr, "No jobs present.\n");
        free(job_array);
        return 1;
    }

    sort_jobs_by_arrival(job_array, n);

    reset_jobs(job_array, n);
    run_scheduler(job_array, n);          
    display_scheduler_report(job_array, n, "Preemptive Shortest Job First");

    reset_jobs(job_array, n);
    run_scheduler(job_array, n);          
    display_scheduler_report(job_array, n, "Round Robin");

    reset_jobs(job_array, n);
    run_scheduler(job_array, n);          
    display_scheduler_report(job_array, n, "Multi-Level Feedback Queue");

    free(job_array);
    return 0;
}