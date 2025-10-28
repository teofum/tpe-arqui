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

void io_init();

void io_blank_from(uint32_t x);

uint32_t io_writes(uint32_t fd, const char *str);
uint32_t io_write(uint32_t fd, const char *str, uint32_t len);

void io_clear();

uint32_t io_read(uint32_t fd, char *buf, uint32_t len);

void io_setfont(vga_font_t font);

void io_setcursor(io_cursor_t cursor);

void io_movecursor(int32_t dx);

vga_framebuffer_t io_get_default_framebuffer();

#endif
