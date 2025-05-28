#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h>
#include <stdint.h>

int strcmp(const char *a, const char *b);

const char *strsplit(char *out, const char *str, char delim);

#endif
