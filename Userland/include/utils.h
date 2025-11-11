#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define lengthof(x) (sizeof((x)) / sizeof((x)[0]))

uint32_t parse_uint(const char *s);

#endif
