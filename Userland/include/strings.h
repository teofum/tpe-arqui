#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint64_t count;
  char *const *strings;
} split_result_t;

int strcmp(const char *a, const char *b);

uint64_t strlen(const char *s);

int strcpy(char *dst, const char *src);

int memcpy(char *dst, const char *src, size_t len);

split_result_t strsplit(const char *str, char delim);

#endif
