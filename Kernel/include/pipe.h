#ifndef PIPE_H
#define PIPE_H

#include <fd.h>
#include <stdint.h>
#include <types.h>

typedef enum {
  PIPE_WRITE,
  PIPE_READ,
} pipe_end_t;

typedef struct pipe_cdt_t *pipe_t;

pipe_t pipe_create();

fd_t pipe_connect(pipe_t pipe, pipe_end_t end);

void pipe_disconnect(pipe_t pipe, pipe_end_t end);

uint32_t pipe_read(pipe_t pipe, char *buf, uint32_t len);

uint32_t pipe_write(pipe_t pipe, char *buf, uint32_t len);

#endif
