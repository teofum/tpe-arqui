#include <fd.h>
#include <mem.h>
#include <pipe.h>
#include <semaphores.h>
#include <stdint.h>

#define PIPE_SIZE 1024

#define advance(x) x = (x + 1) % PIPE_SIZE

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

int32_t pipe_read(pipe_t pipe, char *buf, uint32_t len) {
  if (!pipe->can_read) return 0;

  uint32_t read = 0;
  while (read < len) {
    if (pipe->write_cursor == pipe->read_cursor && !pipe->can_write) break;

    sem_wait(pipe->sem);
    *buf++ = pipe->data[pipe->read_cursor];
    advance(pipe->read_cursor);
    read++;
  }

  return read;
}

int32_t pipe_write(pipe_t pipe, const char *buf, uint32_t len) {
  // Don't allow writing to a pipe if the reading end is closed
  if (!pipe->can_write) return -1;

  uint32_t written = 0;
  while (written < len) {
    pipe->data[pipe->write_cursor] = *buf++;
    advance(pipe->write_cursor);
    sem_post(pipe->sem);
    written++;
  }

  return written;
}
