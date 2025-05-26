#ifndef IO_H
#define IO_H

#include <stdint.h>

void io_putc(char c);

uint32_t io_writes(const char *str);
uint32_t io_write(const char *str, uint32_t len);

void io_clear();

char io_getc();

uint32_t io_read(char *str, uint32_t len);

#endif
