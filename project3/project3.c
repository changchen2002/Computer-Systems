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
    case JOB_NEW:
        return "NEW";
    case JOB_READY:
        return "READY";
    case JOB_RUNNING:
        return "RUNNING";
    case JOB_WAITING:
        return "WAITING";
    case JOB_FINISHED:
        return "FINISHED";
    default:
        return "UNKNOWN";
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
           clock_time,
           job->id,
           job_state_name(job->state),
           event);
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

static void print_summary(Job jobs[], int job_count)
{
    int i;

    puts("\nSummary");
    puts("-------");
    for (i = 0; i < job_count; i++) {
        printf("job=%d arrival=%d total=%d completion=%d turnaround=%d response=%d\n",
               jobs[i].id,
               jobs[i].arrival_time,
               jobs[i].total_cpu_time,
               jobs[i].completion_time,
               jobs[i].completion_time - jobs[i].arrival_time,
               jobs[i].first_run_time - jobs[i].arrival_time);
    }
}

static void run_scheduler(Job jobs[], int job_count)
{
    JobQueue ready_queue;
    JobQueue waiting_queue;
    Job *current_job = NULL;

    queue_init(&ready_queue);
    queue_init(&waiting_queue);

    clock_time = 0;
    os_srand(1);

    add_new_incoming_jobs(jobs, job_count, &ready_queue);

    while (jobs_remaining(jobs, job_count, &ready_queue, &waiting_queue, current_job)) {
        if (current_job == NULL) {
            current_job = choose_job_to_execute(&ready_queue);
        }

        add_new_incoming_jobs(jobs, job_count, &ready_queue);
        process_waiting_jobs(&waiting_queue, &ready_queue);

        if (current_job == NULL) {
            printf("clock=%2d cpu=IDLE\n", clock_time);
            clock_time++;
            continue;
        }

        current_job->remaining_time--;
        current_job->time_in_quantum++;

        printf("clock=%2d job=%d state=%-8s event=cpu-burst remaining=%d\n",
               clock_time,
               current_job->id,
               job_state_name(current_job->state),
               current_job->remaining_time);

        if (current_job->remaining_time == 0) {
            mark_finished(current_job);
            current_job = NULL;
            clock_time++;
            continue;
        }

        if (IO_request()) {
            move_to_waiting(&waiting_queue, current_job, "io-request");
            current_job = NULL;
            clock_time++;
            continue;
        }

        if (current_job->time_in_quantum >= TIME_QUANTUM) {
            move_to_ready(&ready_queue, current_job, "time-slice-expired");
            current_job = NULL;
            clock_time++;
            continue;
        }

        clock_time++;
    }

    print_summary(jobs, job_count);
}

int main(void)
{
    Job jobs[] = {
        { .id = 1, .arrival_time = 0, .total_cpu_time = 7, .remaining_time = 7, .priority = 2, .time_in_quantum = 0, .first_run_time = -1, .completion_time = -1, .state = JOB_NEW, .next = NULL },
        { .id = 2, .arrival_time = 2, .total_cpu_time = 5, .remaining_time = 5, .priority = 1, .time_in_quantum = 0, .first_run_time = -1, .completion_time = -1, .state = JOB_NEW, .next = NULL },
        { .id = 3, .arrival_time = 4, .total_cpu_time = 6, .remaining_time = 6, .priority = 3, .time_in_quantum = 0, .first_run_time = -1, .completion_time = -1, .state = JOB_NEW, .next = NULL }
    };

    run_scheduler(jobs, (int)(sizeof(jobs) / sizeof(jobs[0])));
    return 0;
}
