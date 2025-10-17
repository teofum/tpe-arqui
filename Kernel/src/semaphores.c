#include <lib.h>
#include <mem.h>
#include <scheduler.h>
#include <semaphores.h>

typedef struct sem_cdt_t {
  int value;
  pqueue_t waiters;
} sem_cdt_t;


sem_t sem_create(int initial) {
  sem_t new_sem = (sem_t) mem_alloc(sizeof(sem_cdt_t));
  if (!new_sem) return NULL;

  new_sem->value = initial;
  new_sem->waiters = pqueue_create();

  return new_sem;
}

int sem_down(sem_t sem) {
  _cli();

  if (sem->value == 0) {
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

  if (sem->value++ == 0 && !pqueue_empty(sem->waiters)) {
    scheduler_enqueue(pqueue_dequeue(sem->waiters));
    proc_control_block_t *pcb = &proc_control_table[proc_running_pid];
    pcb->state = PROC_STATE_RUNNING;
  }
  _sti();
  return 0;
}


void sem_close(sem_t sem) {
  pqueue_destroy(sem->waiters);
  mem_free(sem);
}

int sem_candown(sem_t sem) { return (sem->value == 0) ? 0 : 1; }
