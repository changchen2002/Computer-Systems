# CS 5600 Project 2 – Staircase Simulation

## 1. Group Information
Group 2  
Byunghyun Ko, Chang Chen, Yuchong Zhang  

---

## 2. Description
In this project, we implemented a multithreaded staircase simulation using POSIX threads. Each thread represents a customer trying to go either up or down a staircase.

The staircase has a limited number of steps, and multiple threads are allowed on it at the same time as long as they are moving in the same direction. If threads from opposite directions try to enter at the same time, they must wait to avoid conflicts.

The main goals of this project were:
- Prevent deadlock
- Prevent starvation
- Allow multiple threads to move efficiently in the same direction

Each thread randomly chooses a direction, waits if necessary, simulates walking by sleeping, and then exits. We also measure turnaround time for each thread.

---

## 3. Implementation

### Main Idea
We treat the staircase as a shared resource with:
- A maximum capacity (`steps`)
- A current direction (`cur_dir`)
- Counters to track waiting threads and fairness

We use:
- A mutex (`pthread_mutex_t`) to protect shared data
- Condition variables to manage waiting threads

---

### Functions

#### `enter_stairs(int dir, int id)`
This function is called when a thread wants to enter the stairs.
- Locks the mutex
- Checks if the thread needs to wait (wrong direction, full capacity, fairness rules)
- Waits using condition variables if needed
- Updates shared variables when entering
- Prints a message

---

#### `exit_stairs(int dir, int id)`
This function is called when a thread leaves the stairs.
- Locks the mutex
- Decreases occupancy
- Updates direction and fairness if needed
- Wakes up waiting threads
- Prints a message

---

#### `is_blocked(int d)`
Helper function to decide whether a thread should wait.
- Blocks if opposite direction is active
- Blocks if fairness policy requires switching
- Helps prevent starvation

---

#### `thread_function(void *vinfo)`
This is what each thread runs.
- Records start time
- Calls `enter_stairs`
- Sleeps to simulate walking
- Calls `exit_stairs`
- Records end time and computes turnaround time

---

#### `main`
- Reads input or generates random values
- Creates threads with random directions
- Waits for all threads to finish
- Prints turnaround times and average
- Frees memory and destroys mutex

---
## 4. Testing

---
## 5. Deadlock and Starvation

### Deadlock
Deadlock is avoided because:
- Only one mutex is used
- Threads don’t hold multiple locks
- No circular waiting

---

### Starvation
We prevent starvation by:
- Using a `turn` variable to switch direction when needed
- Limiting how many threads from one direction can go in a row (`b_max`)
- Making sure waiting threads eventually get a chance

---

### Verification
We ran stress tests with maximum values and confirmed:
- All threads finish
- No thread gets stuck waiting forever

---

## 6. Performance

We measured turnaround time for each thread using `clock_gettime`.

Typical average turnaround time: ~6–10 seconds (depends on number of threads and steps)

### Improvements
- Allowing multiple threads in the same direction improves performance
- Batch control reduces unnecessary switching
- Threads don’t wait unless necessary

---

## 7. Compile and Run

---

## 8. Contributions

**Chang Chen**
- Designed synchronization logic
- Implemented direction control and fairness
- Wrote `enter_stairs` and `exit_stairs`

**Byunghyun Ko**
- Implemented thread creation and simulation
- Handled random inputs and timing
- Integrated everything together

**Yuchong Zhang**
- Created test cases
- Verified deadlock and starvation conditions
- Measured performance and wrote analysis