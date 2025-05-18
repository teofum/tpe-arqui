#ifndef SYSCALL_DISPATCHER_H
#define SYSCALL_DISPATCHER_H

#include <stdint.h>

#define MAX_SYSCALLS 256

void initSyscalls();
void registerSyscall(uint64_t id, void *syscall);

#endif

