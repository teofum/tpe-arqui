#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pqueue.h>
#include <process.h>


typedef struct {
  int value;
  pqueue_t waiters;
} ksem_t;

typedef ksem_t *sem_t;


sem_t sem_create(int initial);

int sem_down(sem_t);

int sem_up(sem_t);

void sem_close(sem_t);

#endif
