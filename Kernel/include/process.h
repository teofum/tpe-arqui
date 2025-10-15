#ifndef PROCESS_H
#define PROCESS_H

#include <fd.h>
#include <pqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff
#define IDLE_PID 0

#define FD_COUNT 64

#define RETURN_KILLED -1

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

  uint64_t argc;
  char *const *argv;
  const char *description;
  proc_state_t state;
  int return_code;

  pqueue_t waiting_processes;
  uint32_t n_waiting_processes;

  fd_t file_descriptors[FD_COUNT];

  priority_t priority;

  int waiting_for_foreground;
} proc_control_block_t;

typedef struct {
  const char *description;
  pid_t pid;
  proc_state_t state;
  priority_t priority;
  uint64_t rsp;

  int foreground : 1;
} proc_info_t;

extern proc_control_block_t proc_control_table[];

extern pid_t proc_running_pid;

/*
 * Kernel-only function. Initialize the process system. Spawns a process from
 * kernel, without the usual plumbing to keep the calling process running.
 * Used to start the first "init" process as well as an idle process.
 */
void proc_init(proc_entrypoint_t entry_point);

/*
 * Yields control to the scheduler.
 */
void proc_yield();

/*
 * Kernel-only function. Blocks the current process and yields control to the
 * scheduler. NOTE: the process should add itself to some queue before blocking
 * or it may stay in a blocked state indefinitely.
 */
void proc_block();

/*
 * Blocks a process by PID and yields control to the scheduler.
 */
void proc_blockpid(pid_t pid);

/*
 * Set a blocked process to running. The process is likely to block itself
 * again (eg if it's waiting on i/o or foreground)
 */
void proc_runpid(pid_t pid);

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

/*
 * If the current process is running in the background, block until it's brought
 * to the foreground
 */
void proc_wait_for_foreground();

/*
 * Get information about a process. Returns 0 if the process does not exist.
 */
int proc_info(pid_t pid, proc_info_t *out_info);

/*
 * Kernel only function, get the contents of a file descriptor.
 */
fd_t proc_get_fd(uint32_t fd);

#endif
