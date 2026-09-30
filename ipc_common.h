#ifndef IPC_COMMON_H
#define IPC_COMMON_H

#include <sys/types.h>

#define KEY_PATH      "/tmp/pipeline_keyfile"
#define SHM_PROJ_ID   'S'
#define SEM_PROJ_ID   'M'

#define QUEUE_CAPACITY 8
#define MSG_PAYLOAD_SIZE 64

// Semaphore indices within the set
#define SEM_MUTEX 0   // binary: protects head/tail/count
#define SEM_EMPTY 1   // counting: number of free slots
#define SEM_FULL  2   // counting: number of filled slots

typedef struct {
    int  seq;
    char payload[MSG_PAYLOAD_SIZE];
} message_t;

typedef struct {
    message_t buffer[QUEUE_CAPACITY];
    int head;
    int tail;
    int count;   // not strictly required (semaphores track this) but useful for debugging/asserts
} shared_queue_t;

#endif