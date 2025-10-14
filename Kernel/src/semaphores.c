#include <mem.h>
#include <semaphores.h>


sem_t sem_create(int initial) {
  sem_t newSem = (sem_t) mem_alloc(sizeof(ksem_t));
  if (!newSem) return NULL;

  newSem->value = initial;
  newSem->waiters = pqueue_create();

  return newSem;
}

int sem_trydown(sem_t) {}

int sem_down(sem_t) {}


int sem_up(sem_t) {}

//int sem_getvalue(sem_t, int *out){}

int sem_close(sem_t) {}