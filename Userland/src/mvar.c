#include <mvar.h>
#include <print.h>
#include <process.h>
#include <rng.h>
#include <semaphores.h>
#include <stdint.h>
#include <utils.h>

#define INTERVAL_MAX 20

typedef struct {
  char c;
  sem_t mutex;
} mvar_shared_t;

static mvar_shared_t shared;
static char *const alphabet = "ABCDEFGHIJKLMNOPQRSTUVXYZ";
static char *const colors[] = {"",         COL_RED,     COL_GREEN, COL_BLUE,
                               COL_YELLOW, COL_MAGENTA, COL_CYAN};

int writer(uint64_t argc, char *const *argv) {
  pcg32_random_t rng;
  // seed rng with argv address which is different for each process
  pcg32_srand(&rng, argv[1][0], (uint32_t) (uint64_t) argv);

  uint32_t interval = pcg32_rand(&rng) % INTERVAL_MAX;

  while (1) {
    while (interval-- > 0) { yield(); }
    interval = pcg32_rand(&rng) % INTERVAL_MAX;

    sem_wait(shared.mutex);
    if (shared.c == 0) { shared.c = argv[1][0]; }
    sem_post(shared.mutex);
  }

  return 0;
}

int reader(uint64_t argc, char *const *argv) {
  pcg32_random_t rng;
  // seed rng with argv address which is different for each process
  pcg32_srand(&rng, argv[1][4], (uint32_t) (uint64_t) argv);

  uint32_t interval = pcg32_rand(&rng) % INTERVAL_MAX;

  while (1) {
    while (interval-- > 0) { yield(); }
    interval = pcg32_rand(&rng) % INTERVAL_MAX;

    sem_wait(shared.mutex);
    if (shared.c != 0) {
      printf("%s%c", argv[1], shared.c);
      shared.c = 0;
    }
    sem_post(shared.mutex);
  }

  return 0;
}

int mvar(uint64_t argc, char *const *argv) {
  shared.c = 0;
  shared.mutex = sem_create(1);

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
