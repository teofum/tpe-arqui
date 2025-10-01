#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <process.h>

extern int scheduler_force_next;

pid_t scheduler_next();

void scheduler_enqueue(pid_t pid);

#endif
