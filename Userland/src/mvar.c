#include <mvar.h>
#include <print.h>
#include <process.h>
#include <semaphores.h>
#include <stdint.h>
#include <utils.h>

typedef struct {
  char c;
  uint32_t reader_count;
  sem_t reader_count_lock;
  sem_t write_lock;
  sem_t mutex;
} mvar_shared_t;

static mvar_shared_t shared;
static char *const alphabet = "ABCDEFGHIJKLMNOPQRSTUVXYZ";
static char *const colors[] = {"",         COL_RED,     COL_GREEN, COL_BLUE,
                               COL_YELLOW, COL_MAGENTA, COL_CYAN};

int writer(uint64_t argc, char *const *argv) {
  while (1) {
    sem_wait(shared.write_lock);
    sem_wait(shared.mutex);
    sem_post(shared.write_lock);

    shared.c = argv[1][0];

    sem_post(shared.mutex);
  }

  return 0;
}

int reader(uint64_t argc, char *const *argv) {
  while (1) {
    sem_wait(shared.write_lock);
    sem_post(shared.write_lock);

    sem_wait(shared.reader_count_lock);
    if (shared.reader_count++ == 0) sem_wait(shared.mutex);
    sem_post(shared.reader_count_lock);

    printf("%s%c", argv[1], shared.c);

    sem_wait(shared.reader_count_lock);
    if (--shared.reader_count == 0) sem_post(shared.mutex);
    sem_post(shared.reader_count_lock);
  }

  return 0;
}

int mvar(uint64_t argc, char *const *argv) {
  shared.c = 0;
  shared.reader_count = 0;
  shared.mutex = sem_create(1);
  shared.reader_count_lock = sem_create(1);
  shared.write_lock = sem_create(1);

  if (argc < 3) {
    printf("Usage: mvar <n_writers> <n_readers>\n");
    return 1;
  }

  uint32_t writers = parse_uint(argv[1]);
  uint32_t readers = parse_uint(argv[2]);

  if (writers == 0 || writers > 26) {
    printf("mvar: writers must be a number between 1 and 26\n");
    return 1;
  }

  if (readers == 0 || readers > 7) {
    printf("mvar: readers must be a number between 1 and 7\n");
    return 1;
  }

  for (int i = 0; i < writers; i++) {
    char *const writer_argv[2] = {"writer", &alphabet[i]};
    proc_spawn(writer, 2, writer_argv, NULL);
  }

  for (int i = 0; i < readers; i++) {
    char *const reader_argv[2] = {"reader", colors[i]};
    proc_spawn(reader, 2, reader_argv, NULL);
  }

  return 0;
}
