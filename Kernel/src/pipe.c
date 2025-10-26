#include <fd.h>
#include <mem.h>
#include <pipe.h>
#include <semaphores.h>
#include <stdint.h>

#define PIPE_SIZE 1024

typedef struct pipe_cdt_t {
  uint32_t can_read : 1;
  uint32_t can_write : 1;

  uint32_t read_cursor;
  uint32_t write_cursor;
  sem_t sem;

  uint8_t data[PIPE_SIZE];
} pipe_cdt_t;

pipe_t pipe_create() {
  pipe_t pipe = mem_alloc(sizeof(struct pipe_cdt_t));
  if (!pipe) return NULL;

  pipe->can_read = 0;
  pipe->can_write = 0;
  pipe->read_cursor = 0;
  pipe->write_cursor = 0;
  pipe->sem = sem_create(0);

  return pipe;
}

fd_t pipe_connect(pipe_t pipe, pipe_end_t end) {
  switch (end) {
    case PIPE_READ:
      pipe->can_read = 1;
      return create_pipe_fd(pipe);
    case PIPE_WRITE:
      pipe->can_write = 1;
      return create_pipe_fd(pipe);
  }
}

void pipe_disconnect(pipe_t pipe, pipe_end_t end) {
  switch (end) {
    case PIPE_READ:
      pipe->can_read = 0;
      break;
    case PIPE_WRITE:
      pipe->can_write = 0;
      break;
  }

  if (!pipe->can_read && !pipe->can_write) {
    sem_close(pipe->sem);
    mem_free(pipe);
  }
}

uint32_t pipe_read(pipe_t pipe, char *buf, uint32_t len) {
  if (!pipe->can_read) return 0;

  return 0;//TODO
  // Advance read head until we read len bytes
  // If we hit write head...
  //    Block if write end is still connected
  //    Otherwise, destroy the pipe and return (EOF)
}

uint32_t pipe_write(pipe_t pipe, char *buf, uint32_t len) {
  // Don't allow writing to a pipe if the reading end is closed
  if (!pipe->can_write || !pipe->can_read) return 0;

  return 0;//TODO
  // Advance write head until we wrote len bytes
  // If we hit read head, block until read
}
