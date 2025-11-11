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
  sem_t bytes_available;
  sem_t space_available;

  uint8_t data[PIPE_SIZE];
} pipe_cdt_t;

static pipe_t named_pipes[MAX_NAMED_PIPES];

pipe_t pipe_create() {
  pipe_t pipe = mem_alloc(sizeof(struct pipe_cdt_t));
  if (!pipe) return NULL;

  pipe->can_read = 0;
  pipe->can_write = 0;
  pipe->read_cursor = 0;
  pipe->write_cursor = 0;
  pipe->bytes_available = sem_create(0);
  pipe->space_available = sem_create(PIPE_SIZE);

  return pipe;
}

pipe_t pipe_create_named(uint32_t name) {
  if (name >= MAX_NAMED_PIPES || named_pipes[name]) return NULL;

  pipe_t pipe = pipe_create();
  if (pipe) named_pipes[name] = pipe;

  return pipe;
}

fd_t pipe_connect(pipe_t pipe, fd_mode_t end) {
  switch (end) {
    case FD_READ:
      pipe->can_read = 1;
      return create_pipe_fd(pipe, FD_READ);
    case FD_WRITE:
      pipe->can_write = 1;
      return create_pipe_fd(pipe, FD_WRITE);
  }
  return (fd_t) {};
}

fd_t pipe_connect_named(uint32_t name, fd_mode_t end) {
  if (name >= MAX_NAMED_PIPES || !named_pipes[name]) return (fd_t) {};
  return pipe_connect(named_pipes[name], end);
}

int pipe_disconnect(pipe_t pipe, fd_mode_t end) {
  switch (end) {
    case FD_READ:
      pipe->can_read = 0;
      break;
    case FD_WRITE:
      pipe->can_write = 0;
      break;
  }

  if (!pipe->can_read && !pipe->can_write) {
    sem_close(pipe->bytes_available);
    sem_close(pipe->space_available);
    mem_free(pipe);

    return 1;
  }

  return 0;
}

void pipe_disconnect_named(uint32_t name, fd_mode_t end) {
  if (name >= MAX_NAMED_PIPES || !named_pipes[name]) return;

  int destroyed = pipe_disconnect(named_pipes[name], end);
  if (destroyed) named_pipes[name] = NULL;
}

int32_t pipe_read(pipe_t pipe, char *buf, uint32_t len) {
  if (!pipe->can_read) return 0;

  uint32_t read = 0;
  while (read < len) {
    if (pipe->write_cursor == pipe->read_cursor && !pipe->can_write) break;

    sem_wait(pipe->bytes_available);
    *buf++ = pipe->data[pipe->read_cursor];
    advance(pipe->read_cursor);
    sem_post(pipe->space_available);
    read++;
  }

  return read;
}

int32_t pipe_write(pipe_t pipe, const char *buf, uint32_t len) {
  if (!pipe->can_write) return -1;

  uint32_t written = 0;
  while (written < len) {
    sem_wait(pipe->space_available);
    pipe->data[pipe->write_cursor] = *buf++;
    advance(pipe->write_cursor);
    sem_post(pipe->bytes_available);
    written++;
  }

  return written;
}
