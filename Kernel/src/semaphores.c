#include <lib.h>
#include <mem.h>
#include <scheduler.h>
#include <semaphores.h>

struct ksem_s_t {
  int value;
  pqueue_t waiters;
} ksem_s_t;

typedef struct ksem_s_t *ksem_t;

#define MAX_SEMAPHORES 100
// totalmente arbitrario/ maybe cambia a vla

static ksem_t sem_references[MAX_SEMAPHORES] = {0};

int sr_get_first_free() {
  for (int i = 0; i < MAX_SEMAPHORES; ++i) {
    if (sem_references[i] == NULL) { return i; }
  }
  return -1;
}

sem_t sem_create(int initial) {
  ksem_t new_sem = (ksem_t) mem_alloc(sizeof(ksem_s_t));
  if (!new_sem) return -1;

  new_sem->value = initial;
  new_sem->waiters = pqueue_create();

  int index = sr_get_first_free();

  sem_references[index] = new_sem;
  return index;
}

int sem_wait(sem_t sem) {
  _cli();

  ksem_t cur_sem = sem_references[sem];

  if (cur_sem->value == 0) {
    pqueue_enqueue(cur_sem->waiters, proc_running_pid);
    _sti();
    proc_block();
    _cli();
  }

  cur_sem->value--;
  _sti();
  return 0;
}


int sem_post(sem_t sem) {
  _cli();

  ksem_t cur_sem = sem_references[sem];

  if (cur_sem->value++ == 0 && !pqueue_empty(cur_sem->waiters)) {
    scheduler_enqueue(pqueue_dequeue(cur_sem->waiters));
    proc_control_block_t *pcb = &proc_control_table[proc_running_pid];
    pcb->state = PROC_STATE_RUNNING;
  }
  _sti();
  return 0;
}


void sem_close(sem_t sem) {
  ksem_t cur_sem = sem_references[sem];
  pqueue_destroy(cur_sem->waiters);
  mem_free(cur_sem);
  sem_references[sem] = NULL;
}

int sem_willblock(sem_t sem) {
  return (sem_references[sem]->value == 0) ? 1 : 0;
}
