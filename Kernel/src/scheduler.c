#include <pqueue.h>
#include <process.h>
#include <scheduler.h>
#include <types.h>

#define next(x) x = (x + 1) % (MAX_PID + 1)

typedef struct {
  pqueue_t groups[MAX_PRIORITY + 1];
  priority_t next;
} scheduler_priority_list_t;

#define ORDER_SIZE 15
static priority_t order[ORDER_SIZE] = {0, 0, 1, 0, 1, 2, 0, 1,
                                       2, 3, 0, 1, 2, 3, 4};


static scheduler_priority_list_t spl = {0};
int scheduler_force_next = 0;

void scheduler_init() {
  for (int i = 0; i <= MAX_PRIORITY; ++i) { spl.groups[i] = pqueue_create(); }
  spl.next = 0;
}

pid_t scheduler_next() {
  pqueue_t scheduler_queue;
  priority_t next = order[spl.next];
  spl.next = (spl.next + 1) % (ORDER_SIZE);

  for (int i = next; i <= MAX_PRIORITY; ++i) {
    scheduler_queue = spl.groups[i];

    if (!pqueue_empty(scheduler_queue)) {
      pid_t next_pid = pqueue_dequeue(scheduler_queue);

      proc_running_pid = next_pid;
      return next_pid;
    }
  }

  proc_running_pid = 0;
  return 0;
}

void scheduler_enqueue(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];
  pqueue_t scheduler_queue = spl.groups[pcb->priority];

  pqueue_enqueue(scheduler_queue, pid);
}

void scheduler_remove(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];
  pqueue_t scheduler_queue = spl.groups[pcb->priority];

  pqueue_remove_all(scheduler_queue, pid);
}
