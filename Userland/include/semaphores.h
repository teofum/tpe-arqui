#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef int sem_t;


sem_t sem_create(int initial);

int sem_willblock(sem_t sem);

int sem_wait(sem_t sem);

int sem_post(sem_t sem);

void sem_close(sem_t sem);

#endif
