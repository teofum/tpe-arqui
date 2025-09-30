#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Support up to 4096 processes
 */
#define MAX_PID 0xfff

typedef int16_t pid_t;
typedef int (*proc_entrypoint_t)(uint64_t argc, const char **argv);

/*
 * Spawn a process.
 */
pid_t proc_spawn(
  proc_entrypoint_t entry_point, uint64_t argc, const char **argv
);

/*
 * Terminate the current process.
 */
void proc_exit(int return_code);

/*
 * Terminate the current process.
 */
int proc_wait(pid_t pid);

#endif
