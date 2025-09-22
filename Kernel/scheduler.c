#include <scheduler.h>

#define next(x) x = (x + 1) % (MAX_PID + 1)

typedef struct {
  pid_t data[MAX_PID + 1];
  int write_pos, read_pos;
} scheduler_queue_t;

static scheduler_queue_t scheduler_queue = {0};

pid_t scheduler_next() {
  if (scheduler_queue.write_pos == scheduler_queue.read_pos) {
    return 0;// idle process
  }

  pid_t next_pid = scheduler_queue.data[scheduler_queue.read_pos];
  next(scheduler_queue.read_pos);

  return next_pid;
}

void scheduler_enqueue(pid_t pid) {
  scheduler_queue.data[scheduler_queue.write_pos] = pid;
  next(scheduler_queue.write_pos);

  if (scheduler_queue.write_pos == scheduler_queue.read_pos)
    next(scheduler_queue.read_pos);// todo crash and die if queue full
}
