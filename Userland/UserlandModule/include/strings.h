#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h>
#include <stdint.h>

int strcmp(const char *a, const char *b);

const char *strsplit(char *out, const char *str, char delim);

int strcpy(char *dst, const char *src);

int memcpy(char *dst, const char *src, size_t len);

#endif
