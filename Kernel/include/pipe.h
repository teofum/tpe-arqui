#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>
#include <types.h>

typedef enum {
  PIPE_WRITE,
  PIPE_READ,
} pipe_end_t;

typedef struct pipe_cdt_t *pipe_t;

pipe_t pipe_create();

pipe_t pipe_connect(pipe_t pipe, pipe_end_t end);

pipe_t pipe_disconnect(pipe_t pipe, pipe_end_t end);

uint32_t pipe_read(char *buf, uint32_t len);

uint32_t pipe_write(char *buf, uint32_t len);

#endif
