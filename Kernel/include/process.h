#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff

typedef int16_t pid_t;
typedef void (*proc_entrypoint_t)();

typedef enum { PROC_MODE_USER = 0, PROC_MODE_KERNEL } proc_mode_t;

typedef struct {
  void *stack;
  uint64_t rsp;
} proc_control_block_t;

extern proc_control_block_t proc_control_table[];

extern pid_t proc_running_pid;

/*
 * Kernel-only function. Spawns a process from kernel, without the usual plumbing
 * to keep the calling process running. Used to start the first "init" process.
 */
void proc_init(proc_entrypoint_t entry_point);

/*
 * Spawn a process. Returns the PID of the new process.
 */
void proc_spawn(proc_entrypoint_t entry_point, pid_t *new_pid);

/*
 * Terminate the current process.
 */
void proc_exit(int return_code);

/*
 * Wait for a process to end. Blocks the caller until the waited process
 * terminates.
 */
void proc_wait(pid_t pid);

/*
 * Kill a running process.
 */
void proc_kill(pid_t pid);

#endif
