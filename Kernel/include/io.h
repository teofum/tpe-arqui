#ifndef IO_H
#define IO_H

#include <stdint.h>

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

void io_init();

void io_putc(char c);

uint32_t io_writes(const char *str);
uint32_t io_write(const char *str, uint32_t len);

void io_clear();

uint32_t io_read(char *buf, uint32_t len);

void io_setfont(io_font_t font);

#endif
