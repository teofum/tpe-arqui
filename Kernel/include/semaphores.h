#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pqueue.h>
#include <process.h>

typedef struct sem_cdt_t *sem_t;


sem_t sem_create(int initial);

int sem_down(sem_t);

int sem_candown(sem_t);

int sem_up(sem_t);

void sem_close(sem_t);

#endif
