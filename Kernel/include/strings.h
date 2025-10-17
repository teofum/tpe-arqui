#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h>
#include <stdint.h>

int strcmp(const char *a, const char *b);

uint64_t strlen(const char *s);

int strcpy(char *dst, const char *src);

#endif
