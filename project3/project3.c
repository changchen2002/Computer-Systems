#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHANCE_OF_IO_REQUEST 10
#define CHANCE_OF_IO_COMPLETE 4

#define RR_TIME_QUANTUM 3

#define MLFQ_LEVELS 3
#define MLFQ_Q0_QUANTUM 1
#define MLFQ_Q1_QUANTUM 2
#define MLFQ_Q2_QUANTUM 4
#define MLFQ_BOOST_INTERVAL 10

typedef enum {
    JOB_NEW,
    JOB_READY,
    JOB_RUNNING,
    JOB_WAITING,
    JOB_FINISHED
} JobState;

typedef enum {
    POLICY_PSJF,
    POLICY_RR,
    POLICY_MLFQ
} SchedulerPolicy;

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

    int activeT;
    int waitT;
    int sleepT;

    int mlfq_level;
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
    return (os_rand() % CHANCE_OF_IO_REQUEST == 0);
}

int IO_complete(void)
{
    return (os_rand() % CHANCE_OF_IO_COMPLETE == 0);
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

static void mark_finished(Job *job)
{
    job->state = JOB_FINISHED;
    job->completion_time = clock_time + 1;
    job->time_in_quantum = 0;
    log_transition(job, "completed");
}

static void update_job_metrics(Job *job_list, int total_count)
{
    int idx;
    for (idx = 0; idx < total_count; idx++) {
        Job *j = &job_list[idx];
        if (j->state == JOB_READY) {
            j->waitT++;
        } else if (j->state == JOB_WAITING) {
            j->sleepT++;
        } else if (j->state == JOB_RUNNING) {
            j->activeT++;
        }
    }
}

static void order_jobs_by_start(Job *arr, int count)
{
    int p, s, smallest_index;
    Job hold;

    for (p = 0; p < count - 1; p++) {
        smallest_index = p;
        for (s = p + 1; s < count; s++) {
            if (arr[s].arrival_time < arr[smallest_index].arrival_time)
                smallest_index = s;
            else if (arr[s].arrival_time == arr[smallest_index].arrival_time
                     && arr[s].id < arr[smallest_index].id)
                smallest_index = s;
        }
        if (smallest_index != p) {
            hold                = arr[p];
            arr[p]              = arr[smallest_index];
            arr[smallest_index] = hold;
        }
    }
}

static void prepare_jobs_for_run(Job *arr, int count)
{
    int x;
    Job *p;

    for (x = 0; x < count; x++) {
        p = &arr[x];
        p->state           = JOB_NEW;
        p->next            = NULL;
        p->completion_time = -1;
        p->first_run_time  = -1;
        p->remaining_time  = p->total_cpu_time;
        p->activeT         = 0;
        p->sleepT          = 0;
        p->waitT           = 0;
        p->time_in_quantum = 0;
        p->mlfq_level      = 0;
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
        sum_wait       += work_list[i].waitT;
        sum_sleep      += work_list[i].sleepT;
    }

    printf("\n%s scheduler\n\n", scheduler_name);

    printf("     Job# | Total time       | Total time       | Total time\n");
    printf("          | in ready to run  | in sleeping on   | in system\n");
    printf("          | state            | I/O state        |\n");
    printf("==========+==================+==================+==========\n");

    for (i = 0; i < n_jobs; i++) {
        int in_system = work_list[i].completion_time - work_list[i].arrival_time;
        printf("pid%-6d| %-17d| %-17d| %d\n",
               work_list[i].id,
               work_list[i].waitT,
               work_list[i].sleepT,
               in_system);
    }

    printf("==========+==================+==================+==========\n");
    printf("Total simulation run time: %d\n", max_time);
    printf("Total number of jobs: %d\n", n_jobs);
    printf("Shortest job completion time: %d\n", min_turnaround);
    printf("Longest job completion time: %d\n", max_turnaround);
    printf("Average job completion time: %d\n", (int)(sum_turnaround / n_jobs));
    printf("Average time in ready queue: %d\n", (int)(sum_wait / n_jobs));
    printf("Average time sleeping on I/O state: %d\n", (int)(sum_sleep / n_jobs));
}

static int jobs_remaining(Job jobs[], int job_count,
                          const JobQueue *ready_queue,
                          const JobQueue *waiting_queue,
                          const Job *current_job,
                          JobQueue mlfq_queues[],
                          SchedulerPolicy policy)
{
    int i;

    if (policy == POLICY_MLFQ) {
        for (i = 0; i < MLFQ_LEVELS; i++) {
            if (!queue_is_empty(&mlfq_queues[i])) {
                return 1;
            }
        }
    } else {
        if (!queue_is_empty(ready_queue)) {
            return 1;
        }
    }

    if (!queue_is_empty(waiting_queue) || current_job != NULL) {
        return 1;
    }

    for (i = 0; i < job_count; i++) {
        if (jobs[i].state != JOB_FINISHED) {
            return 1;
        }
    }
    return 0;
}

static Job *psjf_pick_best(JobQueue *ready_queue)
{
    Job *best, *cur, *prev, *best_prev;

    if (ready_queue->head == NULL) return NULL;

    best = ready_queue->head;
    cur  = ready_queue->head->next;

    while (cur != NULL) {
        if (cur->remaining_time < best->remaining_time ||
           (cur->remaining_time == best->remaining_time && cur->id < best->id)) {
            best = cur;
        }
        cur = cur->next;
    }

    prev      = NULL;
    best_prev = NULL;
    cur       = ready_queue->head;

    while (cur != best) {
        prev = cur;
        cur  = cur->next;
    }
    best_prev = prev;

    if (best_prev == NULL)
        ready_queue->head = best->next;
    else
        best_prev->next = best->next;

    if (ready_queue->tail == best)
        ready_queue->tail = best_prev;

    best->next = NULL;
    ready_queue->length--;
    return best;
}

static void move_to_ready_policy(JobQueue *ready_queue,
                                 JobQueue mlfq_queues[],
                                 Job *job,
                                 const char *reason,
                                 SchedulerPolicy policy,
                                 int is_new_arrival)
{
    job->state = JOB_READY;
    job->time_in_quantum = 0;

    if (policy == POLICY_PSJF) {
        enqueue(ready_queue, job);
    } else if (policy == POLICY_RR) {
        enqueue(ready_queue, job);
    } else {
        if (is_new_arrival) {
            job->mlfq_level = 0;
        }
        enqueue(&mlfq_queues[job->mlfq_level], job);
    }

    log_transition(job, reason);
}

static void move_to_waiting(JobQueue *waiting_queue, Job *job, const char *reason)
{
    job->state = JOB_WAITING;
    job->time_in_quantum = 0;
    enqueue(waiting_queue, job);
    log_transition(job, reason);
}

static void add_new_incoming_jobs(Job jobs[], int job_count,
                                  JobQueue *ready_queue,
                                  JobQueue mlfq_queues[],
                                  SchedulerPolicy policy)
{
    int i;
    for (i = 0; i < job_count; i++) {
        if (jobs[i].state == JOB_NEW && jobs[i].arrival_time <= clock_time) {
            move_to_ready_policy(ready_queue, mlfq_queues, &jobs[i], "arrived", policy, 1);
        }
    }
}

static void collect_sorted_by_pid(Job *completed[], int *count, Job *job)
{
    int pos = *count;

    while (pos > 0 && completed[pos - 1]->id > job->id) {
        completed[pos] = completed[pos - 1];
        pos--;
    }
    completed[pos] = job;
    (*count)++;
}

static void process_waiting_jobs(JobQueue *waiting_queue,
                                 JobQueue *ready_queue,
                                 JobQueue mlfq_queues[],
                                 SchedulerPolicy policy)
{
    Job *completed_arr[1024];
    int  completed_count = 0;
    Job *prev = NULL;
    Job *cur  = waiting_queue->head;

    while (cur != NULL) {
        Job *next = cur->next;

        if (IO_complete()) {
            if (prev == NULL)
                waiting_queue->head = next;
            else
                prev->next = next;

            if (waiting_queue->tail == cur)
                waiting_queue->tail = prev;

            waiting_queue->length--;
            cur->next = NULL;

            collect_sorted_by_pid(completed_arr, &completed_count, cur);
        } else {
            prev = cur;
        }
        cur = next;
    }

    int k;
    for (k = 0; k < completed_count; k++) {
        move_to_ready_policy(ready_queue, mlfq_queues,
                             completed_arr[k], "io-complete", policy, 0);
    }
}

static Job *choose_job_to_execute(JobQueue *ready_queue,
                                  JobQueue mlfq_queues[],
                                  SchedulerPolicy policy)
{
    Job *job = NULL;

    if (policy == POLICY_PSJF) {
        job = psjf_pick_best(ready_queue);
    } else if (policy == POLICY_RR) {
        job = dequeue(ready_queue);
    } else {
        int lvl = 0;
        while (lvl < MLFQ_LEVELS) {
            if (!queue_is_empty(&mlfq_queues[lvl])) {
                job = dequeue(&mlfq_queues[lvl]);
                break;
            }
            lvl++;
        }
    }

    if (job != NULL) {
        job->state = JOB_RUNNING;
        if (job->first_run_time < 0) {
            job->first_run_time = clock_time;
        }
        log_transition(job, "dispatched");
    }

    return job;
}

static int should_preempt_psjf(const Job *active_job, const JobQueue *ready_queue)
{
    const Job *cur;

    if (active_job == NULL) return 0;

    cur = ready_queue->head;
    while (cur != NULL) {
        if (cur->remaining_time < active_job->remaining_time ||
           (cur->remaining_time == active_job->remaining_time &&
            cur->id < active_job->id)) {
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

static int mlfq_quantum_for_level(int level)
{
    if (level == 0) return MLFQ_Q0_QUANTUM;
    if (level == 1) return MLFQ_Q1_QUANTUM;
    return MLFQ_Q2_QUANTUM;
}

static void mlfq_priority_boost(JobQueue mlfq_queues[],
                                JobQueue *waiting_queue,
                                Job *active_job)
{
    int i;

    for (i = 1; i < MLFQ_LEVELS; i++) {
        while (!queue_is_empty(&mlfq_queues[i])) {
            Job *j = dequeue(&mlfq_queues[i]);
            j->mlfq_level      = 0;
            j->time_in_quantum = 0;
            enqueue(&mlfq_queues[0], j);
        }
    }

    Job *curr = waiting_queue->head;
    while (curr != NULL) {
        curr->mlfq_level = 0;
        curr->time_in_quantum = 0;
        curr = curr->next;
    }

    if (active_job != NULL) {
        active_job->mlfq_level      = 0;
        active_job->time_in_quantum = 0;
    }
}

static void run_scheduler(Job jobs[], int job_count, SchedulerPolicy policy)
{
    JobQueue ready_q, waiting_q;
    JobQueue mlfq_q[MLFQ_LEVELS];
    Job *active_job = NULL;
    int i;

    queue_init(&ready_q);
    queue_init(&waiting_q);
    for (i = 0; i < MLFQ_LEVELS; i++) {
        queue_init(&mlfq_q[i]);
    }

    clock_time = 0;
    os_srand(1);

    while (jobs_remaining(jobs, job_count, &ready_q, &waiting_q, active_job, mlfq_q, policy)) {

        process_waiting_jobs(&waiting_q, &ready_q, mlfq_q, policy);

        add_new_incoming_jobs(jobs, job_count, &ready_q, mlfq_q, policy);

        if (policy == POLICY_MLFQ &&
            clock_time > 0 &&
            clock_time % MLFQ_BOOST_INTERVAL == 0) {
            mlfq_priority_boost(mlfq_q, &waiting_q, active_job);
        }

        if (policy == POLICY_PSJF && active_job != NULL &&
            should_preempt_psjf(active_job, &ready_q)) {
            move_to_ready_policy(&ready_q, mlfq_q, active_job, "preempted", policy, 0);
            active_job = NULL;
        } else if (policy == POLICY_MLFQ && active_job != NULL) {
            int top = 0;
            while (top < MLFQ_LEVELS && queue_is_empty(&mlfq_q[top])) top++;
            if (top < active_job->mlfq_level) {
                move_to_ready_policy(&ready_q, mlfq_q, active_job, "preempted", policy, 0);
                active_job = NULL;
            }
        }

        if (!active_job) {
            active_job = choose_job_to_execute(&ready_q, mlfq_q, policy);
        }

        if (!active_job) {
            printf("clock=%2d cpu=IDLE\n", clock_time);
            update_job_metrics(jobs, job_count);
            clock_time++;
            continue;
        }

        active_job->remaining_time--;
        active_job->time_in_quantum++;

        printf("clock=%2d job=%d state=%-8s event=cpu-burst remaining=%d\n",
               clock_time, active_job->id, job_state_name(active_job->state),
               active_job->remaining_time);

        update_job_metrics(jobs, job_count);

        if (active_job->remaining_time == 0) {
            mark_finished(active_job);
            active_job = NULL;
        } else {
            if (IO_request()) {
                move_to_waiting(&waiting_q, active_job, "io-request");
                active_job = NULL;
            } else if (policy == POLICY_RR &&
                active_job->time_in_quantum >= RR_TIME_QUANTUM) {
                move_to_ready_policy(&ready_q, mlfq_q, active_job,
                                     "time-slice-expired", policy, 0);
                active_job = NULL;
            } else if (policy == POLICY_MLFQ &&
                       active_job->time_in_quantum >=
                       mlfq_quantum_for_level(active_job->mlfq_level)) {
                if (active_job->mlfq_level < MLFQ_LEVELS - 1) {
                    active_job->mlfq_level++;
                }
                move_to_ready_policy(&ready_q, mlfq_q, active_job,
                                     "time-slice-expired", policy, 0);
                active_job = NULL;
            }
        }

        clock_time++;
    }
}

static int load_jobs_from_input(Job **out_ptr)
{
    char buffer[256];
    int n = 0, size = 16;
    Job *list = malloc(sizeof(Job) * size);

    if (list == NULL) {
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), stdin)) {
        int p, a, s, pr;
        if (buffer[0] == '\n' || buffer[0] == '#' || buffer[0] == '\r') continue;

        if (sscanf(buffer, "%d:%d:%d:%d", &p, &a, &s, &pr) == 4) {
            if (n >= size) {
                Job *tmp;
                size *= 2;
                tmp = realloc(list, sizeof(Job) * size);
                if (tmp == NULL) {
                    free(list);
                    exit(1);
                }
                list = tmp;
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
            list[n].waitT           = 0;
            list[n].sleepT          = 0;
            list[n].activeT         = 0;
            list[n].mlfq_level      = 0;
            n++;
        }
    }

    *out_ptr = list;
    return n;
}

int main(int argc, char *argv[])
{
    Job *job_array = NULL;
    int n;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <sjf|rr|mlfq>\n", argv[0]);
        return 1;
    }

    n = load_jobs_from_input(&job_array);

    if (n == 0) {
        fprintf(stderr, "No jobs loaded.\n");
        free(job_array);
        return 1;
    }

    order_jobs_by_start(job_array, n);

    if (strcmp(argv[1], "sjf") == 0) {
        prepare_jobs_for_run(job_array, n);
        run_scheduler(job_array, n, POLICY_PSJF);
        display_scheduler_report(job_array, n, "Preemptive Shortest Job First");
    } else if (strcmp(argv[1], "rr") == 0) {
        prepare_jobs_for_run(job_array, n);
        run_scheduler(job_array, n, POLICY_RR);
        display_scheduler_report(job_array, n, "Round Robin");
    } else if (strcmp(argv[1], "mlfq") == 0) {
        prepare_jobs_for_run(job_array, n);
        run_scheduler(job_array, n, POLICY_MLFQ);
        display_scheduler_report(job_array, n, "Multi-Level Feedback Queue");
    } else {
        fprintf(stderr, "Unknown policy: %s\n", argv[1]);
        free(job_array);
        return 1;
    }

    free(job_array);
    return 0;
}