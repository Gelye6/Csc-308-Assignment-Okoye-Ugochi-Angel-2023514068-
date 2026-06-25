# Okoye-Ugochi-Angel-2023514069-

# CSC 308 – Operating Systems Practicals
# Nnamdi Azikiwe University (UNIZIK)**


# Overview

This repository contains four C programs demonstrating core process synchronization and inter-process communication (IPC) concepts covered in of CSC 308 – Operating Systems.

| # | File | Topic |
|---|------|-------|
| 1 | `p1_mutex.c` | Mutex Lock Demonstration |
| 2 | `p2_prodcons.c` | Producer-Consumer Simulation |
| 3 | `p3_semaphore.c` | Semaphore vs Mutex Comparison |
| 4 | `p4_sharedmem.c` | Shared Memory IPC |


# Environment

- **OS:** Linux (Ubuntu recommended)
- **Compiler:** GCC
- **Libraries:** POSIX Threads (`-lpthread`), POSIX Semaphores, System V IPC


# Compile & Run

# Session 1 – Mutex Lock
```bash
gcc -Wall -o p1_mutex p1_mutex.c -lpthread
./p1_mutex
```
**What to observe:** With mutex, counter is always exact. Without mutex, the result differs every run — this is a race condition.


# Session 2 – Producer-Consumer
```bash
gcc -Wall -o p2_prodcons p2_prodcons.c -lpthread
./p2_prodcons
```
**What to observe:** Producer fills the buffer; consumer drains it. When consumer is slower, buffer fills up and producer waits automatically. No overflow or deadlock.


# Session 3 – Semaphore vs Mutex
```bash
gcc -Wall -o p3_semaphore p3_semaphore.c -lpthread
./p3_semaphore
```
**What to observe:** Both mutex and binary semaphore produce correct results. Counting semaphore enforces that at most 3 threads are active simultaneously — never more.


# Session 4 – Shared Memory
```bash
gcc -Wall -o p4_sharedmem p4_sharedmem.c
./p4_sharedmem
```
*(No `-lpthread` needed — uses System V IPC and `fork()`)*

**What to observe:** Child writes messages into shared memory; parent reads them in real time. Shared memory is properly cleaned up at the end.



# Key Functions Reference

| Function | Session | Purpose |
|----------|---------|---------|
| `pthread_mutex_init/lock/unlock/destroy` | 1, 3 | Mutex lifecycle |
| `sem_init/wait/post/destroy` | 2, 3 | POSIX semaphore operations |
| `shmget()` | 4 | Create shared memory segment |
| `shmat()` | 4 | Attach segment to process |
| `shmdt()` | 4 | Detach segment from process |
| `shmctl(IPC_RMID)` | 4 | Delete segment from kernel |
| `fork()` | 4 | Create child process |


# Concepts Covered

- **Race conditions** and why they occur
- **Mutex locks** for binary mutual exclusion
- **POSIX semaphores** — binary and counting variants
- **Bounded buffer** — circular queue with semaphore-based flow control
- **Shared memory IPC** — fastest form of inter-process communication in Linux
- **Deadlock awareness** — how correct semaphore ordering prevents it
