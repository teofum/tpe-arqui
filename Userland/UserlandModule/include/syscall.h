#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef enum {
  SYS_READ = 0x03,
  SYS_WRITE = 0x04,
  SYS_WRITES = 0x07,
  SYS_PUTC = 0x08,
  SYS_CLEAR = 0x09,
  SYS_FONT = 0x0A,
} syscall_t;

typedef enum {
  IO_FONT_DEFAULT = 0,
  IO_FONT_TINY,
  IO_FONT_TINY_BOLD,
  IO_FONT_SMALL,
  IO_FONT_LARGE,
  IO_FONT_ALT,
  IO_FONT_ALT_BOLD,
  IO_FONT_FUTURE,
  IO_FONT_OLD,
} io_font_t;

extern uint64_t _syscall(uint64_t n, ...);

#endif
