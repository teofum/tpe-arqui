#include <syscallDispatcher.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_SYSCALLS 16

static syscall_t syscalls[MAX_SYSCALLS] = { NULL };

void initSyscalls() {
  // registerSyscall(3, read);
  // registerSyscall(4, write);
  // etc.
}

void registerSyscall(uint64_t syscallID, syscall_t function) {
  if (syscallID < MAX_SYSCALLS)
    syscalls[syscallID] = function;
}

uint64_t syscallDispatcher(uint64_t syscallID, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
  if (syscallID < MAX_SYSCALLS && syscalls[syscallID] != NULL)
    return syscalls[syscallID](arg1, arg2, arg3, arg4, arg5, arg6);
}

