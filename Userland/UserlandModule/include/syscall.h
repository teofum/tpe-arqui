#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef enum {
  SYS_READ = 0x03,
  SYS_WRITE = 0x04,
  SYS_WRITES = 0x05,
  SYS_PUTC = 0x06,
  SYS_CLEAR = 0x07,
} syscall_t;

extern uint64_t _syscall(uint64_t n, ...);

#endif
