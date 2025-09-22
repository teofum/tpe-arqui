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

void proc_spawn(proc_entrypoint_t entry_point);

#endif
