1. Group2: Byunghyun Ko, Chang Chen, Yuchong Zhang
2. Description:
   This project implements a multithreaded stair-crossing simulation using POSIX threads and mutex synchronization. Multiple threads (1-30) attempt to traverse stairs simultaneously, each simulating a crossing time (1-13 seconds). The implementation uses a single mutex lock to ensure mutual exclusion—only one thread can be on the stairs at any time, regardless of direction. Each thread randomly chooses a direction (up or down) and follows the enter_stairs/exit_stairs protocol to safely access the shared stair resource. The goal is to demonstrate deadlock-free, starvation-free concurrent programming with proper synchronization primitives.

- Functions and its purpose:
  - enter_stairs: Acquires the mutex lock before a thread enters the stairs. Ensures only one thread can be on the stairs at a time regardless of direction. Prints a message indicating which thread is entering and in which direction.
  - exit_stairs: Releases the mutex lock after a thread completes traversing the stairs. Prints a message indicating which thread is exiting and in which direction. Allows the next waiting thread to enter.
  - thread_function: The main routine for each thread. Calls enter_stairs to acquire the lock, simulates walking on stairs with sleep(sleep_time), then calls exit_stairs to release the lock and notify other waiting threads.
  - main: Initializes random seed, generates random number of threads (1-30) and steps (1-13). Allocates memory for thread info structures, creates all threads with random directions (0=down, 1=up), waits for all threads to complete using pthread_join, and performs cleanup by freeing memory and destroying the mutex.

- How you tested your project and list the test cases:
   

- How you guarantee that your code is free of deadlock and starvation:
   Deadlock Prevention: Our design uses a single mutex with no circular dependencies. Each thread acquires one lock, performs work, then releases it. Since there is only one resource (the mutex) and no condition variables creating wait chains, circular wait conditions are impossible.
   
   Starvation Prevention: POSIX mutexes implement fair FIFO scheduling at the kernel level. Waiting threads are queued and served in order of arrival, ensuring no thread is indefinitely starved. The mutex_lock operation is guaranteed to eventually return for each waiting thread. Since we use no priority mechanisms and all threads have equal priority, there are no priority inversions that could cause starvation.
   
   Verification: Deterministic testing with fixed random seeds produces identical execution patterns across runs, confirming no race conditions. Stress testing with maximum threads (30) and maximum sleep time (13 seconds) shows all threads eventually complete successfully.

- Average Turnaround time of the examples you run and how you adjusted your project to make your design “efficient”:

- How to compile, run and test the code:

- Contributions:
Chang — Core concurrency algorithm (the “traffic controller”)
a. Owns the shared state + rules: direction control, fairness policy, semaphores/mutex/conds, and the entry/exit protocol.
b. Defines the API everyone else calls: enter_stairs(dir, id) / exit_stairs(dir, id).

Ben — Thread + simulation harness
a. Parses command-line args (customers ≤ 30, steps ≤ 13), creates threads, assigns random directions, sleeps to simulate crossing time, collects timestamps for turnaround
b. Calls A’s API and prints standardized logs (“should wait”, “crossing now”, etc.)

Yuchong — Testing + measurements
Builds test scenarios (including deterministic seeds), checks deadlock/starvation properties with stress runs, computes average turnaround, and writes the “how we guarantee deadlock/starvation-free” + “how we tuned efficiency” sections.

