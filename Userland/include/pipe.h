#ifndef PIPE_H
#define PIPE_H

#include <fd.h>
#include <stdint.h>

typedef struct pipe_cdt_t *pipe_t;

pipe_t pipe_create();

pipe_t pipe_create_named(uint32_t name);

#endif
