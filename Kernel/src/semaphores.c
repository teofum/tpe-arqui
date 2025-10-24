#include <lib.h>
#include <mem.h>
#include <scheduler.h>
#include <semaphores.h>
#include <spinlock.h>

struct sem_cdt {
  int value;
  lock_t lock;
  pqueue_t waiters;
};

typedef struct sem_cdt *sem_cdt_t;

#define MAX_SEMAPHORES 100
// totalmente arbitrario/ maybe cambia a vla

static sem_cdt_t sem_references[MAX_SEMAPHORES + 1] = {0};

/**
 * Returns 0 if invalid 1 if valid
 */
int valid_sem(sem_t sem) {
  if (sem > MAX_SEMAPHORES || sem < 0 || sem_references[sem] == NULL) return 0;
  return 1;
}

int get_free_sem() {
  for (int i = 0; i < MAX_SEMAPHORES; ++i) {
    if (sem_references[i] == NULL) { return i; }
  }
  return -1;
}


sem_t sem_create(int initial) {
  sem_cdt_t new_sem = (sem_cdt_t) mem_alloc(sizeof(struct sem_cdt));
  if (!new_sem) return NULL;

  new_sem->value = initial;
  new_sem->lock = lock_create();
  new_sem->waiters = pqueue_create();

  int index = get_free_sem();
  if (index == -1 || new_sem->lock == -1 || new_sem->waiters == NULL) {
    sem_close(new_sem);
    return NULL;
  }

  sem_references[index] = new_sem;
  return index;
}

int sem_wait(sem_t sem) {
  if (!valid_sem(sem)) return -1;
  sem_cdt_t curr_sem = sem_references[sem];
  lock_acquire(curr_sem->lock);

  if (curr_sem->value == 0) {
    pqueue_enqueue(curr_sem->waiters, proc_running_pid);
    lock_release(curr_sem->lock);
    proc_block();
    lock_acquire(curr_sem->lock);
  }

  lock_release(curr_sem->lock);
  return 0;
}


int sem_post(sem_t sem) {
  if (!valid_sem(sem)) return -1;
  sem_cdt_t curr_sem = sem_references[sem];
  lock_acquire(curr_sem->lock);

  if (!pqueue_empty(curr_sem->waiters)) {
    pid_t pid = pqueue_dequeue(curr_sem->waiters);
    scheduler_enqueue(pid);
    (&proc_control_table[pid])->state = PROC_STATE_RUNNING;
  } else {
    ++curr_sem->value;
  }

  lock_release(curr_sem->lock);
  return 0;
}


void sem_close(sem_t sem) {
  if (!valid_sem(sem)) return -1;
  sem_cdt_t curr_sem = sem_references[sem];
  pqueue_destroy(curr_sem->waiters);
  lock_destroy(curr_sem->lock);
  mem_free(curr_sem);
  sem_references[sem] = NULL;
}

int sem_willblock(sem_t sem) {
  if (!valid_sem(sem)) return -1;
  return (sem_references[sem]->value == 0) ? 1 : 0;
}
