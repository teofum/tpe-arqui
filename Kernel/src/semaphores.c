#include <lib.h>
#include <mem.h>
#include <scheduler.h>
#include <semaphores.h>
#include <spinlock.h>

typedef struct {
  int value;
  lock_t lock;
  pqueue_t waiters;
} semaphore_t;

#define MAX_SEMAPHORES 100

static semaphore_t *sem_references[MAX_SEMAPHORES + 1] = {0};

/**
 * Returns 0 if invalid 1 if valid
 */
static int valid_sem(sem_t sem) {
  if (sem > MAX_SEMAPHORES || sem < 0 || sem_references[sem] == NULL) return 0;
  return 1;
}

/**
 * Returns the first available semaphore index
 */
static int get_free_sem() {
  for (int i = 0; i < MAX_SEMAPHORES; ++i) {
    if (sem_references[i] == NULL) { return i; }
  }
  return -1;
}

/**
 * Creates new semaphore and returns handle, NULL if it fails
 */
sem_t sem_create(int initial) {
  int index = get_free_sem();
  if (index == -1) return -1;

  semaphore_t *new_sem = mem_alloc(sizeof(semaphore_t));
  if (!new_sem) return -1;

  new_sem->value = initial;
  new_sem->lock = lock_create();
  if (new_sem->lock == NULL) {
    mem_free(new_sem);
    return -1;
  }

  new_sem->waiters = pqueue_create();
  if (new_sem->lock == NULL) {
    lock_destroy(new_sem->lock);
    mem_free(new_sem);
    return -1;
  }

  sem_references[index] = new_sem;
  return index;
}

/**
 * Attempts to decrement semaphore, will block if unable to
 * returns -1 if invalid
 */
int sem_wait(sem_t sem) {
  if (!valid_sem(sem)) return -1;

  semaphore_t *curr_sem = sem_references[sem];
  lock_acquire(curr_sem->lock);

  if (curr_sem->value == 0) {
    pqueue_enqueue(curr_sem->waiters, proc_running_pid);
    //lock_release(curr_sem->lock);
    //proc_block();
    proc_block_release(curr_sem->lock);
    lock_acquire(curr_sem->lock);
  }

  curr_sem->value--;
  lock_release(curr_sem->lock);

  return 0;
}

/**
 * Will encrese the semaphore, and unblock waiting process
 * returns -1 if invalid
 */
int sem_post(sem_t sem) {
  if (!valid_sem(sem)) return -1;
  semaphore_t *curr_sem = sem_references[sem];
  lock_acquire(curr_sem->lock);

  curr_sem->value++;
  if (!pqueue_empty(curr_sem->waiters)) {
    pid_t pid = pqueue_dequeue(curr_sem->waiters);
    (&proc_control_table[pid])->state = PROC_STATE_RUNNING;
    scheduler_enqueue(pid);
  }

  lock_release(curr_sem->lock);
  return 0;
}

/**
 * Will free all memory and destroy the semaphore
 */
void sem_close(sem_t sem) {
  if (!valid_sem(sem)) return;
  semaphore_t *curr_sem = sem_references[sem];

  pqueue_destroy(curr_sem->waiters);
  lock_destroy(curr_sem->lock);
  mem_free(curr_sem);
  sem_references[sem] = NULL;
}

/**
 * Returns 1 if calling sem_wait() would block, 0 if not
 */
int sem_willblock(sem_t sem) {
  if (!valid_sem(sem)) return 0;
  return (sem_references[sem]->value == 0) ? 1 : 0;
}
