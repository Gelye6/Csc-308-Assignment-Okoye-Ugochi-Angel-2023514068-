/*
 * ============================================================
 *  Institution : Nnamdi Azikiwe University (UNIZIK)
 *  Course      : CSC 308 — Operating Systems
 *  Session     : Practical | Session 2
 *  Topic       : Producer-Consumer Simulation
 * ============================================================
 *
 * OBJECTIVE:
 *   Implement the classic Producer-Consumer problem using
 *   a bounded circular buffer and POSIX semaphores.
 *
 * PROBLEM OVERVIEW:
 *   - Producer generates items and places them into a buffer
 *   - Consumer retrieves items from the buffer
 *   - Buffer has a fixed capacity (bounded)
 *   - Must prevent: buffer overflow, buffer underflow, race conditions
 *
 * THREE SEMAPHORES USED:
 *   mutex_s  (init=1) — mutual exclusion on buffer access
 *   empty_s  (init=N) — tracks available (empty) slots
 *   filled_s (init=0) — tracks occupied (filled) slots
 *
 * KEY FUNCTIONS USED:
 *   sem_init()    — initialize semaphore
 *   sem_wait()    — P operation: decrement (blocks if zero)
 *   sem_post()    — V operation: increment (unblocks waiting thread)
 *   sem_destroy() — release semaphore resources
 *
 * COMPILE: gcc -Wall -o p2_prodcons p2_prodcons.c -lpthread
 * RUN    : ./p2_prodcons
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* ---- Configuration ---- */
#define BUFFER_CAP   5     /* Maximum items in buffer at once  */
#define TOTAL_ITEMS  8     /* Total items to produce/consume   */
#define PROD_DELAY   1     /* Producer speed (seconds)         */
#define CONS_DELAY   2     /* Consumer speed — slower on purpose */

/* ---- Circular buffer ---- */
int  buf[BUFFER_CAP];
int  put_idx = 0;   /* Next write position */
int  get_idx = 0;   /* Next read  position */

/* ---- Semaphores ---- */
sem_t mutex_s;    /* Guards buffer access         */
sem_t empty_s;    /* Counts empty slots           */
sem_t filled_s;   /* Counts filled slots          */

/* ---- Visualise buffer state ---- */
void show_buffer(const char *actor, const char *action, int item, int idx) {
    printf("  %-10s | %-8s item=%-3d slot=%-2d | buf: [",
           actor, action, item, idx);
    for (int i = 0; i < BUFFER_CAP; i++) {
        if (buf[i] > 0) printf("%2d", buf[i]);
        else             printf(" _");
    }
    printf(" ]\n");
}

/*
 * PRODUCER THREAD
 * Generates items numbered 1..TOTAL_ITEMS and inserts them
 * into the circular buffer, waiting when the buffer is full.
 */
void *producer(void *arg) {
    for (int item = 1; item <= TOTAL_ITEMS; item++) {
        sleep(PROD_DELAY);             /* Simulate production time   */

        sem_wait(&empty_s);            /* Block if no empty slots    */
        sem_wait(&mutex_s);            /* Lock the buffer            */

        /* --- Critical section: write item --- */
        buf[put_idx] = item;
        show_buffer("PRODUCER", "PUT", item, put_idx);
        put_idx = (put_idx + 1) % BUFFER_CAP;
        /* --- End critical section --- */

        sem_post(&mutex_s);            /* Unlock buffer              */
        sem_post(&filled_s);           /* Signal: one more item ready */
    }
    return NULL;
}

/*
 * CONSUMER THREAD
 * Retrieves items from the circular buffer one at a time,
 * waiting when the buffer is empty.
 */
void *consumer(void *arg) {
    for (int i = 0; i < TOTAL_ITEMS; i++) {
        sleep(CONS_DELAY);             /* Consumer is intentionally slower */

        sem_wait(&filled_s);           /* Block if nothing to consume */
        sem_wait(&mutex_s);            /* Lock the buffer             */

        /* --- Critical section: read item --- */
        int item = buf[get_idx];
        buf[get_idx] = 0;              /* Clear the slot              */
        show_buffer("CONSUMER", "GET", item, get_idx);
        get_idx = (get_idx + 1) % BUFFER_CAP;
        /* --- End critical section --- */

        sem_post(&mutex_s);            /* Unlock buffer               */
        sem_post(&empty_s);            /* Signal: one slot now free   */
    }
    return NULL;
}

int main(void) {
    pthread_t prod_t, cons_t;

    printf("\n");
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║  CSC 308 | Session 2: Producer-Consumer ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n\n");

    printf("  Buffer capacity : %d slots\n", BUFFER_CAP);
    printf("  Total items     : %d\n", TOTAL_ITEMS);
    printf("  Producer delay  : %ds  |  Consumer delay: %ds\n\n",
           PROD_DELAY, CONS_DELAY);
    printf("  (Consumer is slower — watch the buffer fill up!)\n\n");

    /* Initialise buffer and semaphores */
    for (int i = 0; i < BUFFER_CAP; i++) buf[i] = 0;
    sem_init(&mutex_s,  0, 1);
    sem_init(&empty_s,  0, BUFFER_CAP);
    sem_init(&filled_s, 0, 0);

    /* Start threads */
    pthread_create(&prod_t, NULL, producer, NULL);
    pthread_create(&cons_t, NULL, consumer, NULL);

    /* Wait for both to finish */
    pthread_join(prod_t, NULL);
    pthread_join(cons_t, NULL);

    /* Cleanup */
    sem_destroy(&mutex_s);
    sem_destroy(&empty_s);
    sem_destroy(&filled_s);

    printf("\n  ✓ All %d items produced and consumed.\n", TOTAL_ITEMS);
    printf("  ✓ No deadlock, no overflow, no underflow.\n\n");
    printf("  OBSERVATION:\n");
    printf("  - When consumer is slower, buffer fills up.\n");
    printf("  - Producer blocks automatically when buffer is full.\n");
    printf("  - Consumer blocks automatically when buffer is empty.\n");
    printf("  - Semaphores handle all of this without busy-waiting.\n\n");

    return 0;
}
