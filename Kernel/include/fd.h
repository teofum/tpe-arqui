#ifndef FD_H
#define FD_H

#include <stdint.h>

#define FD_NONE -1
#define FD_TTY 0

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef int32_t fd_t;

#endif
