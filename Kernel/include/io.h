#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <vga.h>

typedef enum {
  IO_CURSOR_NONE,
  IO_CURSOR_UNDER,
  IO_CURSOR_FRAME,
  IO_CURSOR_BLOCK,
} io_cursor_t;

void io_blank_from(uint32_t x);

void io_putc(char c);

uint32_t io_writes(const char *str);
uint32_t io_write(const char *str, uint32_t len);

void io_clear();

uint32_t io_read(char *buf, uint32_t len);

void io_setfont(vga_font_t font);

void io_setcursor(io_cursor_t cursor);

void io_movecursor(int32_t dx);

#endif
