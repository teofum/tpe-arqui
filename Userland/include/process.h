#ifndef PROCESS_H
#define PROCESS_H

#include <fd.h>
#include <pipe.h>
#include <stddef.h>
#include <stdint.h>
/*
 * Support up to 4096 processes
 */
#define MAX_PID 0x7ff

#define FD_COUNT 64

typedef int priority_t;// TODO: esto no va ak

typedef int16_t pid_t;
typedef int (*proc_entrypoint_t)(uint64_t argc, char *const *argv);

typedef enum {
  PROC_STATE_RUNNING = 0,
  PROC_STATE_BLOCKED,
  PROC_STATE_EXITED,
} proc_state_t;

typedef struct {
  const char *description;
  pid_t pid;
  proc_state_t state;
  uint32_t priority;
  uint64_t rsp;

  int foreground : 1;
} proc_info_t;

typedef struct {
  uint32_t fd;
  fd_type_t type;
  pipe_t pipe;
  pipe_end_t mode;
} proc_fd_descriptor_t;

typedef struct {
  priority_t priority;
  uint32_t n_fds;
  proc_fd_descriptor_t fds[FD_COUNT];
} proc_descriptor_t;


/*
 * Spawn a process.
 */
pid_t proc_spawn(
  proc_entrypoint_t entry_point, uint64_t argc, char *const *argv,
  proc_descriptor_t *desc
);

/*
 * Terminate the current process.
 */
void proc_exit(int return_code);

/*
 * Terminate the current process.
 */
int proc_wait(pid_t pid);

/*
 * Get the PID for the running process.
 */
pid_t getpid();

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
 * Yields control to the scheduler.
 */
void yield();

/*
 * Get information about a process. Returns 0 if the process does not exist.
 */
int proc_info(pid_t pid, proc_info_t *out_info);

/*
 * Blocks a process by PID and yields control to the scheduler.
 */
void proc_block(pid_t pid);

/*
 * Set a blocked process to running. The process is likely to block itself
 * again (eg if it's waiting on i/o or foreground)
 */
void proc_run(pid_t pid);

#endif
