#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <process.h>

#define MAX_PRIORITY 4
#define DEFAULT_PRIORITY 2

typedef int16_t pid_t;// TODO: esto no va ak

extern int scheduler_force_next;

pid_t scheduler_next();

void scheduler_enqueue(pid_t pid);


#endif
