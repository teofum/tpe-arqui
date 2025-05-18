#include <syscallDispatcher.h>

void *syscallDispatchTable[MAX_SYSCALLS];

void initSyscalls() {
  // registerSyscall(3, read);
  // registerSyscall(4, write);
  // etc.
}

void registerSyscall(uint64_t id, void *syscall) {
  if (id < MAX_SYSCALLS)
    syscallDispatchTable[id] = syscall;
}

