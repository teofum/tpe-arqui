#include <mem.h>
#include <semaphores.h>


sem_t sem_create(int initial) {
  sem_t newSem = (sem_t) mem_alloc(sizeof(ksem_t));
  if (!newSem) return NULL;

  newSem->value = initial;
  newSem->waiters = pqueue_create();

  return newSem;
}

int sem_canDown(sem_t sem) {
  if (sem->value == 0) return 0;
  return sem->value;
}

int sem_down(sem_t sem) {
  if (sem_canDown(sem)) {
    sem->value -= 1;
  } else {
    pqueue_enqueue(sem->waiters, pid);// todo
  }
}


int sem_up(sem_t sem) {}// todo

//int sem_getvalue(sem_t, int *out){}

void sem_close(sem_t sem) {
  pqueue_destroy(sem->waiters);// todo verificar esto
  mem_free(sem);
}