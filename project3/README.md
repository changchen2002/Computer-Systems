# CS 5600 Project 3

## Group Information
Group 2  
Byunghyun Ko, Chang Chen, Yuchong Zhang

## Overview
This project implements the core parts of a CPU scheduling simulator in C. The program models job execution over time with a clock-driven main loop, tracks job state transitions, simulates I/O requests and I/O completion, and manages the ready and waiting queues.

The current implementation runs as a standalone simulation with a small built-in workload defined in `main()`.

## Implemented Features
- Clock-based scheduling loop
- Job state management
- Ready queue management
- Waiting queue management for I/O
- I/O request simulation with `IO_request()`
- I/O completion simulation with `IO_complete()`
- Round-robin execution with a fixed time quantum
- Event logging for job arrivals, dispatch, CPU bursts, I/O events, time-slice expiration, and completion
- End-of-run summary with completion, turnaround, and response times

## Job States
Each job moves through the following states:
- `JOB_NEW`
- `JOB_READY`
- `JOB_RUNNING`
- `JOB_WAITING`
- `JOB_FINISHED`

## Scheduling Behavior
The simulator currently uses a round-robin policy:
- Jobs arrive at their configured arrival times
- Arrived jobs enter the ready queue
- The scheduler dispatches the next ready job to the CPU
- A running job may:
  - finish its remaining CPU time
  - request I/O and move to the waiting queue
  - use up its time quantum and return to the ready queue
- Waiting jobs are checked each clock tick to see whether their I/O completes
- If no job is ready to run, the CPU is idle for that tick

## File Structure
- `project3.c`: main scheduler simulator implementation
- `README.md`: project description and usage instructions

## Build
Use `clang` to compile:

```sh
clang -Wall -Wextra -std=c11 project3.c -o project3
```

## Run
After compiling, run:

```sh
./project3
```

## Sample Output
The program prints:
- per-tick scheduling events
- queue/state transitions
- a final summary for each job

## Notes
- The random number generator is seeded with `os_srand(1)` for reproducible results.
- The current workload is hardcoded in `main()`.
- The current time quantum is defined by `TIME_QUANTUM`.
