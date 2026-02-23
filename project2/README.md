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

We wrote a separate deterministic test file `test_deterministic.c` that hardcodes customer directions to produce reproducible results across runs. This allows us to systematically verify correctness without relying on random output.

### Test Cases

| Test | Customers | Stairs | Directions         | Purpose |
|------|-----------|--------|--------------------|---------|
| 1    | 1         | 1      | UP                 | Minimum base case |
| 2    | 2         | 1      | UP, DOWN           | Classic deadlock stress test |
| 3    | 5         | 3      | All UP             | Same-direction efficiency |
| 4    | 6         | 3      | Alternating UP/DOWN| Maximum direction conflict |
| 5    | 6         | 3      | 5 DOWN + 1 UP      | Starvation check (UP minority) |
| 6    | 6         | 3      | 5 UP + 1 DOWN      | Starvation check (DOWN minority) |
| 7    | 10        | 5      | 5 UP + 5 DOWN      | Full stair capacity test |
| 8    | 30        | 13     | Alternating UP/DOWN| Maximum load stress test |

### Results Summary

**Test 1 – Minimum Case**  
1 customer crossed with no waiting. Turnaround: 1.005s.

**Test 2 – Deadlock Stress**  
Customer 0 (UP) entered first, Customer 1 (DOWN) waited and entered after. No deadlock occurred. Average turnaround: 1.507s.

**Test 3 – All Same Direction**  
3 customers entered simultaneously (filling the 3-stair capacity). Remaining 2 waited for a slot, then entered. Average turnaround: 4.205s — demonstrates efficient parallel movement.

**Test 4 – Alternating UP/DOWN**  
All 3 UP customers crossed first, then all 3 DOWN customers crossed. Direction switching happened cleanly. Average turnaround: 4.505s.

**Test 5 – Starvation Check (5 DOWN + 1 UP)**  
The lone UP customer (Customer 5) was granted access after the first batch of DOWN customers finished, before the remaining DOWN customers resumed. The UP customer was not starved. Average turnaround: 5.509s.

**Test 6 – Starvation Check (5 UP + 1 DOWN)**  
Same result mirrored — the lone DOWN customer crossed between UP batches. Average turnaround: 5.507s.

**Test 7 – Full Capacity**  
All 5 UP customers filled the stairs simultaneously, then all 5 DOWN customers did the same. Average turnaround: 7.507s.

**Test 8 – Max Load (30 customers, 13 stairs)**  
15 DOWN customers crossed in the first wave, 13 UP customers in the second, with remaining customers handled in subsequent batches. No deadlock or starvation observed. Average turnaround: 22.974s.

---

## 5. Deadlock and Starvation

### Deadlock
Deadlock is avoided because:
- Only one mutex is used
- Threads don't hold multiple locks
- No circular waiting

---

### Starvation
We prevent starvation by:
- Using a `turn` variable to switch direction when needed
- Limiting how many threads from one direction can go in a row (`b_max`)
- Making sure waiting threads eventually get a chance

---

## 6. Performance

The key efficiency decision in our design is allowing multiple same-direction customers to use the stairs concurrently, up to the stair capacity limit. This reduces total wait time significantly compared to a one-at-a-time approach.

To prevent starvation without sacrificing efficiency, we set `b_max = steps`. This means up to `steps` customers from one direction can enter before the system is forced to switch to the other direction. This value was chosen because it matches the physical capacity of the stairs — once a full batch has crossed, it is a natural point to switch directions.

### Average Turnaround Times from Test Runs

| Test | Customers | Stairs | Avg Turnaround |
|------|-----------|--------|----------------|
| 1    | 1         | 1      | 1.005s         |
| 2    | 2         | 1      | 1.507s         |
| 3    | 5         | 3      | 4.205s         |
| 4    | 6         | 3      | 4.505s         |
| 5    | 6         | 3      | 5.509s         |
| 6    | 6         | 3      | 5.507s         |
| 7    | 10        | 5      | 7.507s         |
| 8    | 30        | 13     | 22.974s        |

The results show that when all customers travel in the same direction (Test 3), turnaround is minimized because there is no waiting for direction switches. Mixed-direction tests (Tests 4–8) have higher averages due to necessary waiting, but the batch-based policy keeps this overhead low by maximizing the number of customers crossing in each wave.

---

## 7. Compile and Run

### Compile

**Main program:**
```bash
gcc -o project2 project2.c -lpthread

```

**Deterministic test file:**
```bash
gcc -o test_deterministic test_deterministic.c -lpthread
```

### Run

**Main program (specify customers and stairs):**
```bash
./project2 <num_customers> <num_stairs>
```
Example:
```bash
./project2 10 5
```

**Main program (random mode — no arguments):**
```bash
./project2
```

**Run all deterministic test cases:**
```bash
./test_deterministic
```

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
- Wrote `test_deterministic.c` with 8 deterministic test scenarios
- Verified deadlock and starvation-free behavior across all test cases
- Measured and analyzed turnaround time performance