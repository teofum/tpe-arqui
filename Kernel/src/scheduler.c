#include <process.h>
#include <scheduler.h>
#include <types.h>

#define next(x) x = (x + 1) % (MAX_PID + 1)

typedef struct {
  pid_t data[MAX_PID + 1];//todo max size
  int write_pos, read_pos;
} scheduler_queue_t;

typedef struct {
  scheduler_queue_t groups[MAX_PRIORITY + 1];
  priority_t cap;
  priority_t next;
} scheduler_priority_list_t;

static scheduler_priority_list_t spl = {0};
int scheduler_force_next = 0;

pid_t scheduler_next() {
  scheduler_queue_t *scheduler_queue;
  priority_t cap = spl.cap;
  priority_t next = spl.next++;
  if (next >= cap) {
    spl.next = 0;
    ++spl.cap;
  }
  if (cap > MAX_PRIORITY) { spl.cap = 0; }

  for (int i = next; i <= cap; ++i) {
    scheduler_queue = &spl.groups[i];

    if (scheduler_queue->write_pos != scheduler_queue->read_pos) {

      pid_t next_pid = scheduler_queue->data[scheduler_queue->read_pos];
      next(scheduler_queue->read_pos);

      proc_running_pid = next_pid;
      return next_pid;
    }
  }

  //todas las queues vacias
  proc_running_pid = 0;
  return 0;// idle process //todo esto noc si existe
}

void scheduler_enqueue(pid_t pid) {
  proc_control_block_t *pcb = &proc_control_table[pid];
  scheduler_queue_t *scheduler_queue = &spl.groups[pcb->priority];

  scheduler_queue->data[scheduler_queue->write_pos] = pid;
  next(scheduler_queue->write_pos);

  if (scheduler_queue->write_pos == scheduler_queue->read_pos)
    next(scheduler_queue->read_pos);// todo crash and die if queue full
}
