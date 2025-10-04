#ifndef PROCESS_H
#define PROCESS_H

#include <pqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff
#define IDLE_PID 0

typedef int (*proc_entrypoint_t)(uint64_t argc, char *const *argv);
typedef int priority_t;

typedef enum {
  PROC_STATE_RUNNING = 0,
  PROC_STATE_BLOCKED,
  PROC_STATE_EXITED,
} proc_state_t;

typedef struct {
  void *stack;
  uint64_t rsp;

  proc_state_t state;
  int return_code;

  pqueue_t waiting_processes;
  uint32_t n_waiting_processes;

  priority_t priority;
} proc_control_block_t;

extern proc_control_block_t proc_control_table[];

extern pid_t proc_running_pid;

/*
 * Kernel-only function. Initialize the process system. Spawns a process from
 * kernel, without the usual plumbing to keep the calling process running.
 * Used to start the first "init" process as well as an idle process.
 */
void proc_init(proc_entrypoint_t entry_point);

/*
 * Kernel-only function. Yields control to the scheduler.
 */
void proc_yield();

/*
 * Kernel-only function. Blocks the current process and yields control to the
 * scheduler. NOTE: the process should add itself to some queue before blocking
 * or it may stay in a blocked state indefinitely.
 */
void proc_block();

/*
 * Spawn a process. Returns the PID of the new process.
 */
pid_t proc_spawn(
  proc_entrypoint_t entry_point, uint64_t argc, char *const *argv,
  priority_t priority
);

/*
 * Terminate the current process.
 */
void proc_exit(int return_code);

/*
 * Wait for a process to end. Blocks the caller until the waited process
 * terminates.
 */
int proc_wait(pid_t pid);

/*
 * Get the running process PID
 */
pid_t proc_getpid();

/*
 * Kill a running process.
 */
void proc_kill(pid_t pid);

#endif
