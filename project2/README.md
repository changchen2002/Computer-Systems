# CS 5600 Project 2 – Staircase Simulation

## 1. Group Information
Group 2  
Byunghyun Ko, Chang Chen, Yuchong Zhang  

---

## 2. Description
This project builds a multithreaded staircase simulation using POSIX threads to represent individual customers navigating a shared space. The core logic allows multiple threads to occupy the staircase at once if they are all traveling in the same direction. Those approaching from the opposite side must wait for the path to clear. This design was created to avoid pitfalls of concurrent programming, such as deadlock and starvation, while maximizing efficiency through parallel movement. In the simulation, each thread randomly chooses a direction and waits for the appropriate signal before simulating its traversal through timed sleep intervals. To evaluate the system's performance, we tracked the turnaround time for every thread from its arrival to its exit.

---

## 3. Implementation

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

## 6. Performance

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