#ifndef PROCESS_H
#define PROCESS_H

#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff

typedef int priority_t;// TODO: esto no va ak

typedef int16_t pid_t;
typedef int (*proc_entrypoint_t)(uint64_t argc, char *const *argv);

/*
 * Spawn a process.
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
 * Terminate the current process.
 */
int proc_wait(pid_t pid);

/*
 * Get the PID for the running process.
 */
pid_t getpid();

#endif
