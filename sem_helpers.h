#ifndef SEM_HELPERS_H
#define SEM_HELPERS_H

#include <sys/sem.h>
#include <stdio.h>

static inline void sem_wait_op(int semid, int sem_num) {
    struct sembuf op = { .sem_num = sem_num, .sem_op = -1, .sem_flg = 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("semop wait");
    }
}

static inline void sem_post_op(int semid, int sem_num) {
    struct sembuf op = { .sem_num = sem_num, .sem_op = +1, .sem_flg = 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("semop post");
    }
}

#endif