#ifndef IO_H
#define IO_H

#include <stdint.h>

void io_init();

void io_putc(char c);

uint32_t io_writes(const char *str);
uint32_t io_write(const char *str, uint32_t len);

void io_clear();

uint32_t io_read(char *buf, uint32_t len);

#endif
