#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "ipc_common.h"
#include "sem_helpers.h"

#define NUM_MESSAGES 20

int main(void) {
    // 1. Derive a shared key from a real file both processes agree on
    key_t shm_key = ftok(KEY_PATH, SHM_PROJ_ID);
    key_t sem_key = ftok(KEY_PATH, SEM_PROJ_ID);
    if (shm_key == -1 || sem_key == -1) { perror("ftok"); return 1; }

    // 2. Create (or attach to) the shared memory segment
    int shmid = shmget(shm_key, sizeof(shared_queue_t), IPC_CREAT | 0666);
    if (shmid == -1) { perror("shmget"); return 1; }

    shared_queue_t *q = (shared_queue_t *)shmat(shmid, NULL, 0);
    if (q == (void *)-1) { perror("shmat"); return 1; }

    // 3. Create the semaphore set (3 semaphores: mutex, empty, full)
    int semid = semget(sem_key, 3, IPC_CREAT | 0666);
    if (semid == -1) { perror("semget"); return 1; }

    // 4. Initialize shared state and semaphore values
    //    (Producer is the designated initializer for this simple example)
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    semctl(semid, SEM_MUTEX, SETVAL, 1);              // unlocked
    semctl(semid, SEM_EMPTY, SETVAL, QUEUE_CAPACITY);  // all slots free
    semctl(semid, SEM_FULL,  SETVAL, 0);               // no items yet

    printf("[Producer] Initialized shared memory and semaphores. Producing...\n");

    // 5. Produce messages
    for (int i = 0; i < NUM_MESSAGES; i++) {
        message_t msg;
        msg.seq = i;
        snprintf(msg.payload, sizeof(msg.payload), "msg-%d", i);

        sem_wait_op(semid, SEM_EMPTY);   // wait for a free slot (blocks if full)
        sem_wait_op(semid, SEM_MUTEX);   // enter critical section

        q->buffer[q->head] = msg;
        q->head = (q->head + 1) % QUEUE_CAPACITY;
        q->count++;

        sem_post_op(semid, SEM_MUTEX);   // leave critical section
        sem_post_op(semid, SEM_FULL);    // signal: one more filled slot

        printf("[Producer] pushed seq=%d\n", i);
        usleep(10000);  // simulate some work between messages
    }

    printf("[Producer] Done producing. Detaching (leaving segment for consumer)...\n");
    shmdt(q);
    // NOTE: deliberately NOT calling IPC_RMID here — consumer still needs it.
    // Consumer will clean up after it finishes reading.
    return 0;
}