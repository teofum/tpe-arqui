#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pqueue.h>
#include <process.h>

typedef int sem_t;

sem_t sem_create(int initial);

int sem_wait(sem_t);

int sem_willblock(sem_t);

int sem_post(sem_t);

void sem_close(sem_t);

#endif
