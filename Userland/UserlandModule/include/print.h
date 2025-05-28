#ifndef PRINT_H
#define PRINT_H
#include <stdarg.h>
#include <stdint.h>

int32_t sprintf(char *buf, const char *fmt, ...);

int32_t printf(const char *fmt, ...);

#endif
