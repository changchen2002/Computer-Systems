#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

// ── Same sync globals as project2.c ─────────────────────────────────────────
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_up   = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_down = PTHREAD_COND_INITIALIZER;

int occupancy = 0;
int cur_dir   = -1;
int waiting[2] = {0, 0};
int turn  = -1;
int b_cnt = 0;
int b_max = 0;
int steps = 0;
int sleep_time = 0;

typedef struct thread_info {
    int id;
    int direction;
    struct timespec start;
    struct timespec end;
    double turnaround;
} thread_info_t;

static double get_dt(struct timespec s, struct timespec e) {
    return (e.tv_sec - s.tv_sec) + (e.tv_nsec - s.tv_nsec) / 1e9;
}

int is_blocked(int d) {
    if (occupancy > 0 && cur_dir != d) return 1;
    if (occupancy == 0 && turn != -1 && turn != d && waiting[turn] > 0) return 1;
    if (occupancy > 0 && cur_dir == d)
        if (waiting[1-d] > 0 && b_cnt >= b_max) return 1;
    return 0;
}

void enter_stairs(int dir, int id) {
    pthread_mutex_lock(&m);
    waiting[dir]++;
    while (is_blocked(dir) || occupancy >= steps) {
        if (dir == 1) pthread_cond_wait(&c_up,   &m);
        else          pthread_cond_wait(&c_down,  &m);
    }
    if (occupancy == 0) { cur_dir = dir; b_cnt = 0; }
    waiting[dir]--;
    occupancy++;
    b_cnt++;
    printf("  [ENTER] Customer %d going %s  [on stairs: %d/%d]\n",
           id, dir==1?"UP":"DOWN", occupancy, steps);
    pthread_mutex_unlock(&m);
}

void exit_stairs(int dir, int id) {
    pthread_mutex_lock(&m);
    occupancy--;
    printf("  [EXIT]  Customer %d finished %s [on stairs: %d/%d]\n",
           id, dir==1?"UP":"DOWN", occupancy, steps);
    if (dir==1) pthread_cond_broadcast(&c_up);
    else        pthread_cond_broadcast(&c_down);
    if (occupancy == 0) {
        cur_dir = -1;
        int other = 1 - dir;
        if      (waiting[other] > 0) turn = other;
        else if (waiting[dir]   > 0) turn = dir;
        else                         turn = -1;
        b_cnt = 0;
        pthread_cond_broadcast(&c_up);
        pthread_cond_broadcast(&c_down);
    }
    pthread_mutex_unlock(&m);
}

void *thread_function(void *vinfo) {
    thread_info_t *info = (thread_info_t *)vinfo;
    clock_gettime(CLOCK_MONOTONIC, &info->start);
    enter_stairs(info->direction, info->id);
    sleep(sleep_time);
    exit_stairs(info->direction, info->id);
    clock_gettime(CLOCK_MONOTONIC, &info->end);
    info->turnaround = get_dt(info->start, info->end);
    pthread_exit(NULL);
}

// ── Helper: reset globals between tests ─────────────────────────────────────
void reset_globals(int s) {
    steps      = s;
    sleep_time = s;
    b_max      = s;
    occupancy  = 0;
    cur_dir    = -1;
    waiting[0] = waiting[1] = 0;
    turn  = -1;
    b_cnt = 0;
}

// ── Helper: run one test scenario ───────────────────────────────────────────
void run_test(const char *name, int *dirs, int n, int s) {
    printf("\n================================================\n");
    printf("TEST: %s\n", name);
    printf("Customers: %d | Stairs: %d\n", n, s);
    printf("Directions: ");
    for (int i = 0; i < n; i++) printf("%s ", dirs[i]==1?"UP":"DOWN");
    printf("\n------------------------------------------------\n");

    reset_globals(s);

    pthread_t      *tids = malloc(sizeof(pthread_t)      * n);
    thread_info_t  *info = malloc(sizeof(thread_info_t)  * n);

    for (int i = 0; i < n; i++) {
        info[i].id        = i;
        info[i].direction = dirs[i];
        pthread_create(&tids[i], NULL, thread_function, &info[i]);
    }

    double total = 0;
    for (int i = 0; i < n; i++) {
        pthread_join(tids[i], NULL);
        printf("  Customer %d turnaround: %.4fs\n", info[i].id, info[i].turnaround);
        total += info[i].turnaround;
    }
    printf("  Average turnaround: %.4fs\n", total / n);
    printf("================================================\n");

    free(tids);
    free(info);
}

// ── Test scenarios ───────────────────────────────────────────────────────────
int main() {

    // Test 1: Minimum — 1 customer, 1 stair
    {
        int dirs[] = {1};
        run_test("Test 1: Minimum (1 customer UP, 1 stair)", dirs, 1, 1);
    }

    // Test 2: Classic deadlock scenario — 2 customers opposite directions, 1 stair
    {
        int dirs[] = {1, 0};
        run_test("Test 2: Deadlock Stress (UP vs DOWN, 1 stair)", dirs, 2, 1);
    }

    // Test 3: All same direction — no conflict expected
    {
        int dirs[] = {1, 1, 1, 1, 1};
        run_test("Test 3: All UP (5 customers, 3 stairs)", dirs, 5, 3);
    }

    // Test 4: Alternating directions — maximum conflict
    {
        int dirs[] = {1, 0, 1, 0, 1, 0};
        run_test("Test 4: Alternating UP/DOWN (6 customers, 3 stairs)", dirs, 6, 3);
    }

    // Test 5: Starvation check — many DOWN, one UP
    {
        int dirs[] = {0, 0, 0, 0, 0, 1};
        run_test("Test 5: Starvation Check (5 DOWN + 1 UP, 3 stairs)", dirs, 6, 3);
    }

    // Test 6: Starvation check reversed — many UP, one DOWN
    {
        int dirs[] = {1, 1, 1, 1, 1, 0};
        run_test("Test 6: Starvation Check (5 UP + 1 DOWN, 3 stairs)", dirs, 6, 3);
    }

    // Test 7: Max stairs capacity — fill stairs completely
    {
        int dirs[] = {1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
        run_test("Test 7: Full Capacity (5 UP + 5 DOWN, 5 stairs)", dirs, 10, 5);
    }

    // Test 8: Max load — 30 customers alternating, 13 stairs
    {
        int dirs[30];
        for (int i = 0; i < 30; i++) dirs[i] = i % 2;
        run_test("Test 8: Max Load (30 customers alternating, 13 stairs)", dirs, 30, 13);
    }

    pthread_mutex_destroy(&m);
    return 0;
}