#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef void *sem_t;

sem_t sem_create(int initial);

int sem_candown(sem_t sem);

int sem_down(sem_t sem);

int sem_up(sem_t sem);

void sem_close(sem_t sem);

#endif
