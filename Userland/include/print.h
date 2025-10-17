#ifndef PRINT_H
#define PRINT_H
#include <stdarg.h>
#include <stdint.h>

#define COL_RESET "\x1A R;"
#define COL_RED "\x1A 240,116,100;"
#define COL_GREEN "\x1A 195,248,132;"
#define COL_BLUE "\x1A 115,173,233;"
#define COL_CYAN "\x1A 56,212,240;"
#define COL_YELLOW "\x1A 255,197,96;"
#define COL_MAGENTA "\x1A 187,123,215;"
#define COL_GRAY "\x1A 160,160,160;"

int32_t sprintf(char *buf, const char *fmt, ...);

int32_t printf(const char *fmt, ...);

#endif
