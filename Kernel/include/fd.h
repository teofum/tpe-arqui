#ifndef FD_H
#define FD_H

#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef enum {
  FD_NONE,
  FD_TTY,
  FD_PIPE,
} fd_type_t;

typedef struct {
  fd_type_t type;
  void *data;
} fd_t;

#define create_empty_fd()                                                      \
  (fd_t) { .type = FD_NONE, .data = NULL }
#define create_tty_fd()                                                        \
  (fd_t) { .type = FD_TTY, .data = NULL }
#define create_pipe_fd(x)                                                      \
  (fd_t) { .type = FD_PIPE, .data = x }

#endif
