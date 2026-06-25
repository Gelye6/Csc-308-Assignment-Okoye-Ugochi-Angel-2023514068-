/*
 * ============================================================
 *  Institution : Nnamdi Azikiwe University (UNIZIK)
 *  Course      : CSC 308 — Operating Systems
 *  Session     : Practical | Session 3
 *  Topic       : Semaphore Implementation in C
 * ============================================================
 *
 * OBJECTIVE:
 *   Compare mutex locks and POSIX semaphores for protecting
 *   shared resources, and demonstrate counting semaphores
 *   for managing a pool of N concurrent resources.
 *
 * THREE EXPERIMENTS:
 *   1. Mutex lock       — binary lock, thread-owned
 *   2. Binary semaphore — binary lock, signal-based (not owned)
 *   3. Counting semaphore — allows up to N threads simultaneously
 *
 * KEY DIFFERENCE:
 *   Mutex  → only the thread that locked it can unlock it
 *   Semaphore → any thread can signal (post), great for signaling
 *   Counting semaphore → models resource pools (printers, DBs, etc.)
 *
 * COMPILE: gcc -Wall -o p3_semaphore p3_semaphore.c -lpthread
 * RUN    : ./p3_semaphore
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

/* ---- Configuration ---- */
#define THREAD_COUNT    6
#define LOOP_COUNT     80000
#define POOL_SIZE       3     /* Counting semaphore: max concurrent threads */

/* ---- Shared counters ---- */
long count_mtx  = 0;
long count_sem  = 0;
int  pool_active = 0;   /* Tracks threads inside the pool */

/* ---- Sync primitives ---- */
pthread_mutex_t  the_mutex;
sem_t            the_binary_sem;
sem_t            the_pool_sem;

/* ============================================================
 *  EXPERIMENT 1: Mutex Lock
 * ============================================================ */
void *exp1_mutex(void *arg) {
    for (int i = 0; i < LOOP_COUNT; i++) {
        pthread_mutex_lock(&the_mutex);
        count_mtx++;
        pthread_mutex_unlock(&the_mutex);
    }
    return NULL;
}

/* ============================================================
 *  EXPERIMENT 2: Binary Semaphore
 * ============================================================ */
void *exp2_binary_sem(void *arg) {
    for (int i = 0; i < LOOP_COUNT; i++) {
        sem_wait(&the_binary_sem);   /* Acquire */
        count_sem++;
        sem_post(&the_binary_sem);   /* Release */
    }
    return NULL;
}

/* ============================================================
 *  EXPERIMENT 3: Counting Semaphore (resource pool)
 *  Only POOL_SIZE threads can be active at any moment.
 * ============================================================ */
void *exp3_pool(void *arg) {
    int tid = *(int *)arg;

    sem_wait(&the_pool_sem);        /* Try to enter pool (block if full) */
    pool_active++;
    printf("    Thread %d → ENTERED pool | active: %d/%d\n",
           tid, pool_active, POOL_SIZE);

    sleep(1);                       /* Simulate work inside pool */

    pool_active--;
    printf("    Thread %d ← LEAVING pool | active: %d/%d\n",
           tid, pool_active, POOL_SIZE);
    sem_post(&the_pool_sem);        /* Release slot back to pool */

    return NULL;
}

/* ---- Timing helper: milliseconds between two timespecs ---- */
double ms_between(struct timespec a, struct timespec b) {
    return (b.tv_sec  - a.tv_sec)  * 1000.0
         + (b.tv_nsec - a.tv_nsec) / 1.0e6;
}

/* ---- Print a formatted result row ---- */
void print_row(const char *label, long result, long expected, double ms) {
    printf("  %-20s | result=%-10ld | correct=%-5s | %.1f ms\n",
           label, result,
           result == expected ? "YES ✓" : "NO ✗",
           ms);
}

int main(void) {
    pthread_t  tids[THREAD_COUNT];
    int        ids[THREAD_COUNT];
    struct timespec t0, t1;
    long expected = (long)THREAD_COUNT * LOOP_COUNT;

    printf("\n");
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║  CSC 308 | Session 3: Semaphore vs Mutex║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n\n");

    printf("  Threads: %d | Loops: %d | Expected total: %ld\n\n",
           THREAD_COUNT, LOOP_COUNT, expected);

    /* ---- Experiment 1: Mutex ---- */
    pthread_mutex_init(&the_mutex, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_create(&tids[i], NULL, exp1_mutex, NULL);
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_join(tids[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    pthread_mutex_destroy(&the_mutex);
    print_row("Mutex Lock", count_mtx, expected, ms_between(t0, t1));

    /* ---- Experiment 2: Binary Semaphore ---- */
    sem_init(&the_binary_sem, 0, 1);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_create(&tids[i], NULL, exp2_binary_sem, NULL);
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_join(tids[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    sem_destroy(&the_binary_sem);
    print_row("Binary Semaphore", count_sem, expected, ms_between(t0, t1));

    /* ---- Experiment 3: Counting Semaphore ---- */
    printf("\n  Counting Semaphore — pool size: %d (watch no more than %d active at once)\n\n",
           POOL_SIZE, POOL_SIZE);
    sem_init(&the_pool_sem, 0, POOL_SIZE);
    for (int i = 0; i < THREAD_COUNT; i++) {
        ids[i] = i + 1;
        pthread_create(&tids[i], NULL, exp3_pool, &ids[i]);
    }
    for (int i = 0; i < THREAD_COUNT; i++) pthread_join(tids[i], NULL);
    sem_destroy(&the_pool_sem);

    printf("\n  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  SUMMARY                                             │\n");
    printf("  ├─────────────────────────────────────────────────────┤\n");
    printf("  │  Mutex            → exclusive lock, thread-owned    │\n");
    printf("  │  Binary Semaphore → exclusive lock, signal-based    │\n");
    printf("  │  Counting Sem     → resource pool of size N         │\n");
    printf("  │                                                      │\n");
    printf("  │  Use mutex when: simple on/off lock is enough       │\n");
    printf("  │  Use counting sem when: managing DB/printer pools   │\n");
    printf("  └─────────────────────────────────────────────────────┘\n\n");

    return 0;
}
