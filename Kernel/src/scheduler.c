#include <process.h>
#include <scheduler.h>
#include <types.h>

#define next(x) x = (x + 1) % (MAX_PID + 1)

typedef struct {
  pid_t data[MAX_PID + 1];//todo max size
  int write_pos, read_pos;
} scheduler_queue_t;

typedef struct {
  scheduler_queue_t groups[MAX_PRIORITY];
} scheduler_priority_list_t;

static scheduler_priority_list_t spl = {0};
int scheduler_force_next = 0;

// todo borrar: {0;rojo} {1;verde}
pid_t scheduler_next() {
  scheduler_queue_t *scheduler_queue;
  for (int i = 0; i < MAX_PRIORITY; ++i) {
    scheduler_queue = &spl.groups[i % MAX_PRIORITY];
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
