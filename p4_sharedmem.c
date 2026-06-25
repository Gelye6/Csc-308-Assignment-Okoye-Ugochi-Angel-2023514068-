/*
 * ============================================================
 *  Institution : Nnamdi Azikiwe University (UNIZIK)
 *  Course      : CSC 308 — Operating Systems
 *  Session     : Practical | Session 4
 *  Topic       : Shared Memory Programming
 * ============================================================
 *
 * OBJECTIVE:
 *   Implement inter-process communication (IPC) using a
 *   shared memory segment accessible by both parent and child.
 *
 * HOW SHARED MEMORY WORKS:
 *   1. shmget()  → kernel allocates a shared memory segment
 *   2. shmat()   → process maps segment into its address space
 *   3. Read/write data using normal pointer operations
 *   4. shmdt()   → detach segment from process address space
 *   5. shmctl()  → remove the segment from the kernel (cleanup)
 *
 * SYNCHRONIZATION:
 *   We use a 'finished' flag inside shared memory to signal
 *   when the writer is done — a simple coordination mechanism.
 *   In production systems, a semaphore would be used instead.
 *
 * APPLICATIONS:
 *   Database buffer pools, web server worker processes,
 *   scientific simulations, real-time sensor data exchange.
 *
 * COMPILE: gcc -Wall -o p4_sharedmem p4_sharedmem.c
 * RUN    : ./p4_sharedmem
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ---- Shared memory config ---- */
#define SHM_KEY     7777
#define MSG_COUNT   5

/*
 * SharedBlock — the structure we place inside shared memory.
 * Both parent and child access this via a pointer after shmat().
 */
typedef struct {
    int  msg_id;           /* Current message number        */
    int  writer_done;      /* Flag: 1 when child is finished */
    char text[300];        /* The actual message payload     */
} SharedBlock;

/* ---- Helper: print a divider ---- */
void divider(void) { printf("  %s\n", "─────────────────────────────────────────"); }

int main(void) {
    int          shm_id;
    SharedBlock *shm;
    pid_t        child;

    printf("\n");
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║  CSC 308 | Session 4: Shared Memory IPC║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n\n");

    /* ============================================================
     * STEP 1: Create shared memory segment
     *   - SHM_KEY  : unique identifier for this segment
     *   - IPC_CREAT: create if it doesn't exist
     *   - 0666     : read/write permissions for all
     * ============================================================ */
    shm_id = shmget(SHM_KEY, sizeof(SharedBlock), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    printf("  [STEP 1] Shared memory segment created.\n");
    printf("           Key=%d | ID=%d | Size=%lu bytes\n\n",
           SHM_KEY, shm_id, sizeof(SharedBlock));

    /* ============================================================
     * STEP 2: Attach to parent process address space
     *   - NULL: let OS choose the virtual address
     *   - 0   : no special flags (read+write)
     * ============================================================ */
    shm = (SharedBlock *)shmat(shm_id, NULL, 0);
    if (shm == (SharedBlock *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    /* Initialise all fields to zero/empty */
    memset(shm, 0, sizeof(SharedBlock));
    printf("  [STEP 2] Segment attached to parent. Memory initialised.\n\n");

    divider();
    printf("  Forking child process...\n");
    divider();

    child = fork();
    if (child < 0) { perror("fork failed"); exit(EXIT_FAILURE); }

    /* ============================================================
     * CHILD PROCESS — Writer
     * Attaches to the same shared segment and writes messages.
     * ============================================================ */
    if (child == 0) {
        shm = (SharedBlock *)shmat(shm_id, NULL, 0);
        if (shm == (SharedBlock *)-1) { perror("child shmat"); exit(1); }

        printf("\n  [CHILD  | PID %d] Writer process started.\n\n", getpid());

        for (int n = 1; n <= MSG_COUNT; n++) {
            shm->msg_id = n;
            snprintf(shm->text, sizeof(shm->text),
                     "Transmission %d of %d — sent by child PID %d",
                     n, MSG_COUNT, getpid());
            printf("  [CHILD  ] WROTE  → \"%s\"\n", shm->text);
            sleep(1);   /* Stagger writes so parent can observe */
        }

        shm->writer_done = 1;
        printf("\n  [CHILD  ] All messages sent. Setting done flag.\n");

        /* STEP 5a: Detach from child */
        shmdt(shm);
        exit(EXIT_SUCCESS);
    }

    /* ============================================================
     * PARENT PROCESS — Reader
     * Polls shared memory and reads each new message as it arrives.
     * ============================================================ */
    printf("\n  [PARENT | PID %d] Reader process waiting...\n\n", getpid());
    sleep(1);   /* Give child a moment to start */

    int last_read = 0;
    while (!shm->writer_done) {
        if (shm->msg_id > last_read) {
            printf("  [PARENT ] READ   ← \"%s\"\n", shm->text);
            last_read = shm->msg_id;
        }
        usleep(200000);   /* Poll every 200ms */
    }

    /* Read any final message after the done flag is set */
    if (shm->msg_id > last_read)
        printf("  [PARENT ] READ   ← \"%s\"\n", shm->text);

    /* Wait for child to exit cleanly */
    wait(NULL);

    printf("\n  [PARENT ] %d messages received. Communication complete.\n", MSG_COUNT);
    divider();

    /* ============================================================
     * STEP 5b: Detach and delete shared memory segment
     *   shmdt()          — unmap from process address space
     *   shmctl(IPC_RMID) — mark segment for deletion by kernel
     * ============================================================ */
    shmdt(shm);
    shmctl(shm_id, IPC_RMID, NULL);
    printf("  [STEP 5] Shared memory detached and removed.\n\n");

    printf("  REAL-WORLD APPLICATIONS OF SHARED MEMORY IPC:\n");
    printf("  ┌──────────────────────────────────────────────────┐\n");
    printf("  │  • Database engines    — shared buffer pools     │\n");
    printf("  │  • Web servers         — worker process queues   │\n");
    printf("  │  • Scientific apps     — parallel data exchange  │\n");
    printf("  │  • Real-time systems   — sensor data pipelines   │\n");
    printf("  │  Note: shared memory + semaphores = high-perf IPC│\n");
    printf("  └──────────────────────────────────────────────────┘\n\n");

    return 0;
}
