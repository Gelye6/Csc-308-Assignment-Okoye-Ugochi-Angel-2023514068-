/*
 * ============================================================
 *  Institution : Nnamdi Azikiwe University (UNIZIK)
 *  Course      : CSC 308 — Operating Systems
 *  Session     : Practical | Session 1
 *  Topic       : Mutex Lock Demonstration
 * ============================================================
 *
 * OBJECTIVE:
 *   Demonstrate how mutex locks enforce mutual exclusion
 *   when multiple threads share a variable concurrently.
 *
 * WHAT IS A MUTEX?
 *   A mutex (mutual exclusion lock) is a synchronization
 *   primitive that ensures only ONE thread can access a
 *   critical section at a time. Without it, concurrent
 *   writes cause race conditions and data corruption.
 *
 * KEY FUNCTIONS USED:
 *   pthread_mutex_init()    — Initialize the mutex
 *   pthread_mutex_lock()    — Acquire the lock (blocks if taken)
 *   pthread_mutex_unlock()  — Release the lock
 *   pthread_mutex_destroy() — Free resources when done
 *
 * COMPILE: gcc -Wall -o p1_mutex p1_mutex.c -lpthread
 * RUN    : ./p1_mutex
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* ---- Configuration ---- */
#define NUM_THREADS     5
#define STEPS_EACH  200000

/* ---- Shared data ---- */
long result_safe   = 0;   /* Protected by mutex          */
long result_unsafe = 0;   /* No protection — race-prone  */

/* ---- Mutex declaration ---- */
pthread_mutex_t mtx;

/*
 * Thread function: increments the shared counter WITH mutex.
 * The lock guarantees that only one thread modifies the
 * variable at a time — no overlap, no lost updates.
 */
void *thread_safe(void *arg) {
    for (int i = 0; i < STEPS_EACH; i++) {
        pthread_mutex_lock(&mtx);   /* --- ENTER critical section --- */
        result_safe++;
        pthread_mutex_unlock(&mtx); /* --- EXIT  critical section --- */
    }
    return NULL;
}

/*
 * Thread function: increments the shared counter WITHOUT mutex.
 * All threads read-modify-write at the same time, causing
 * lost updates. This is called a RACE CONDITION.
 */
void *thread_unsafe(void *arg) {
    for (int i = 0; i < STEPS_EACH; i++) {
        result_unsafe++;            /* Multiple threads clash here */
    }
    return NULL;
}

/* ---- Helper: print a result row ---- */
void print_result(const char *label, long actual, long expected) {
    printf("  %-20s | Result: %-10ld | Expected: %-10ld | %s\n",
           label, actual, expected,
           actual == expected ? "CORRECT ✓" : "WRONG ✗ (race condition)");
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    long expected = (long)NUM_THREADS * STEPS_EACH;

    printf("\n");
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║   CSC 308 | Session 1: Mutex Locks     ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n\n");

    printf("  Threads        : %d\n", NUM_THREADS);
    printf("  Steps/thread   : %d\n", STEPS_EACH);
    printf("  Expected total : %ld\n\n", expected);

    printf("  Running tests...\n\n");

    /* === TEST A: WITH MUTEX === */
    pthread_mutex_init(&mtx, NULL);

    for (int t = 0; t < NUM_THREADS; t++)
        pthread_create(&threads[t], NULL, thread_safe, NULL);
    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(threads[t], NULL);

    pthread_mutex_destroy(&mtx);

    /* === TEST B: WITHOUT MUTEX === */
    for (int t = 0; t < NUM_THREADS; t++)
        pthread_create(&threads[t], NULL, thread_unsafe, NULL);
    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(threads[t], NULL);

    /* === RESULTS === */
    printf("  ┌──────────────────────────────────────────────────────────────┐\n");
    printf("  │                        RESULTS                               │\n");
    printf("  ├──────────────────────────────────────────────────────────────┤\n");
    print_result("With Mutex", result_safe, expected);
    print_result("Without Mutex", result_unsafe, expected);
    printf("  └──────────────────────────────────────────────────────────────┘\n\n");

    printf("  OBSERVATION:\n");
    printf("  - With mutex   : counter is always exact. Threads take turns.\n");
    printf("  - Without mutex: counter is wrong. Threads overwrite each other.\n");
    printf("  - Run this program 5 times — the 'without mutex' value changes\n");
    printf("    every run. This unpredictability is the hallmark of a race\n");
    printf("    condition (non-deterministic behaviour).\n\n");

    return 0;
}
