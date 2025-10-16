#include <lib.h>
#include <mem.h>
#include <scheduler.h>
#include <semaphores.h>


sem_t sem_create(int initial) {
  sem_t newSem = (sem_t) mem_alloc(sizeof(ksem_t));
  if (!newSem) return NULL;

  newSem->value = initial;
  newSem->waiters = pqueue_create();

  return newSem;
}

int sem_down(sem_t sem) {
  _cli();

  while (sem->value == 0) {
    pqueue_enqueue(sem->waiters, proc_running_pid);
    _sti();
    proc_block();
    _cli();
  }

  sem->value--;
  _sti();
  return 0;
}


int sem_up(sem_t sem) {
  _cli();
  if (sem->value++ == 0 && !pqueue_empty(sem->waiters))
    scheduler_enqueue(pqueue_dequeue(sem->waiters));
  _sti();
  return 0;
}


void sem_close(sem_t sem) {
  pqueue_destroy(sem->waiters);
  mem_free(sem);
}