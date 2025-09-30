#include "process.h"
#include <scheduler.h>

#define next(x) x = (x + 1) % (MAX_PID + 1)

typedef struct {
  pid_t data[MAX_PID + 1];//todo max size
  int write_pos, read_pos;
} scheduler_queue_t;

typedef struct {
  scheduler_queue_t groups[MAX_PRIORITY + 1];
  int cur;//temporal
} scheduler_priority_list_t;

static scheduler_priority_list_t spl = {0};

static count = 0;

pid_t scheduler_next() {
  scheduler_queue_t *scheduler_queue = &spl.groups[spl.cur];

  if (scheduler_queue->write_pos == scheduler_queue->read_pos) {
    return 0;// idle process //todo esto noc si existe
  }

  pid_t next_pid = scheduler_queue->data[scheduler_queue->read_pos];
  next(scheduler_queue->read_pos);

  proc_running_pid = next_pid;
  return next_pid;
}

void scheduler_enqueue(pid_t pid) {
  scheduler_queue_t *scheduler_queue = &spl.groups[spl.cur];

  scheduler_queue->data[scheduler_queue->write_pos] = pid;
  next(scheduler_queue->write_pos);

  if (scheduler_queue->write_pos == scheduler_queue->read_pos)
    next(scheduler_queue->read_pos);// todo crash and die if queue full
}
