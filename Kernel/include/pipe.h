#ifndef PIPE_H
#define PIPE_H

#include <fd.h>
#include <stdint.h>

#define MAX_NAMED_PIPES 64

typedef struct pipe_cdt_t *pipe_t;

pipe_t pipe_create();

pipe_t pipe_create_named(uint32_t name);

fd_t pipe_connect(pipe_t pipe, fd_mode_t end);

fd_t pipe_connect_named(uint32_t name, fd_mode_t end);

int pipe_disconnect(pipe_t pipe, fd_mode_t end);

void pipe_disconnect_named(uint32_t name, fd_mode_t end);

int32_t pipe_read(pipe_t pipe, char *buf, uint32_t len);

int32_t pipe_write(pipe_t pipe, const char *buf, uint32_t len);

#endif
