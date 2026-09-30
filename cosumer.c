#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "ipc_common.h"
#include "sem_helpers.h"

#define NUM_MESSAGES 20

int main(void) {
    key_t shm_key = ftok(KEY_PATH, SHM_PROJ_ID);
    key_t sem_key = ftok(KEY_PATH, SEM_PROJ_ID);
    if (shm_key == -1 || sem_key == -1) { perror("ftok"); return 1; }

    // Attach to EXISTING segment/semaphores (no IPC_CREAT needed, but harmless if included
    // since the producer already created them; using it here too for simplicity/robustness
    // in case consumer starts first — see note below)
    int shmid = shmget(shm_key, sizeof(shared_queue_t), 0666);
    if (shmid == -1) { perror("shmget - is the producer running?"); return 1; }

    shared_queue_t *q = (shared_queue_t *)shmat(shmid, NULL, 0);
    if (q == (void *)-1) { perror("shmat"); return 1; }

    int semid = semget(sem_key, 3, 0666);
    if (semid == -1) { perror("semget - is the producer running?"); return 1; }

    printf("[Consumer] Attached. Consuming...\n");

    for (int i = 0; i < NUM_MESSAGES; i++) {
        sem_wait_op(semid, SEM_FULL);    // wait for a filled slot (blocks if empty)
        sem_wait_op(semid, SEM_MUTEX);   // enter critical section

        message_t msg = q->buffer[q->tail];
        q->tail = (q->tail + 1) % QUEUE_CAPACITY;
        q->count--;

        sem_post_op(semid, SEM_MUTEX);   // leave critical section
        sem_post_op(semid, SEM_EMPTY);   // signal: one more free slot

        printf("[Consumer] popped seq=%d payload=%s\n", msg.seq, msg.payload);
    }

    printf("[Consumer] Done. Cleaning up shared memory and semaphores.\n");
    shmdt(q);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}