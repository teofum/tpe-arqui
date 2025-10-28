#ifndef PIPE_H
#define PIPE_H

#include <fd.h>
#include <stdint.h>

typedef enum {
  PIPE_WRITE,
  PIPE_READ,
} pipe_end_t;

typedef struct pipe_cdt_t *pipe_t;

pipe_t pipe_create();

#endif
