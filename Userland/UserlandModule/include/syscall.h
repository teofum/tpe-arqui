#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef enum {
  /* I/O syscalls */
  SYS_READ = 0x03,
  SYS_WRITE = 0x04,
  SYS_WRITES = 0x07,
  SYS_PUTC = 0x08,
  SYS_CLEAR = 0x09,
  SYS_FONT = 0x0A,
  SYS_BLANKLINE = 0x0B,
  SYS_CURSOR = 0x0C,
  SYS_CURMOVE = 0x0D,

  /* Keyboard syscalls */
  SYS_KBD_POLLEVENTS = 0x10,
  SYS_KBD_KEYDOWN = 0x11,
  SYS_KBD_KEYPRESSED = 0x12,
  SYS_KBD_KEYRELEASED = 0x13,
  SYS_KBD_GETKEYEVENT = 0x14,

  /* Video syscalls */
  SYS_VGA_CLEAR = 0x20,
  SYS_VGA_PIXEL = 0x21,
  SYS_VGA_LINE = 0x22,
  SYS_VGA_RECT = 0x23,
  SYS_VGA_FRAME = 0x24,
  SYS_VGA_SHADE = 0x25,
  SYS_VGA_GRADIENT = 0x26,
  SYS_VGA_FONT = 0x27,
  SYS_VGA_TEXT = 0x28,
  SYS_VGA_TEXTWRAP = 0x29,
  SYS_VGA_PRESENT = 0x2A,
  SYS_VGA_SET_FB = 0x2B,
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

typedef enum {
  IO_CURSOR_NONE,
  IO_CURSOR_UNDER,
  IO_CURSOR_FRAME,
  IO_CURSOR_BLOCK,
} io_cursor_t;


extern uint64_t _syscall(uint64_t n, ...);

#endif
