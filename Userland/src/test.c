#include <mem.h>
#include <print.h>
#include <process.h>
#include <semaphores.h>
#include <strings.h>
#include <test.h>
#include <test_util.h>

void *memset(void *dst, int32_t c, uint64_t length);

#define MAX_BLOCKS 128

typedef struct {
  void *address;
  uint32_t size;
} mm_rq;

static int test_mm(uint64_t argc, char *const *argv) {
  mm_rq mm_rqs[MAX_BLOCKS];
  uint8_t rq;
  uint32_t total;
  uint64_t max_memory;

  if (argc != 1) return -1;

  if ((max_memory = satoi(argv[0])) <= 0) return -1;

  while (1) {
    rq = 0;
    total = 0;

    // Request as many blocks as we can
    while (rq < MAX_BLOCKS && total < max_memory) {
      mm_rqs[rq].size = get_uniform(max_memory - total - 1) + 1;
      mm_rqs[rq].address = mem_alloc(mm_rqs[rq].size);

      if (mm_rqs[rq].address) {
        total += mm_rqs[rq].size;
        rq++;
      }
    }

    // Set
    uint32_t i;
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address) memset(mm_rqs[i].address, i, mm_rqs[i].size);

    // Check
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
          printf("test_mm ERROR\n");
          return -1;
        }

    // Free
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address) mem_free(mm_rqs[i].address);
  }
}

#define SEM_ID 42
#define TOTAL_PAIR_PROCESSES 2

static int64_t global;

static void slow_inc(int64_t *p, int64_t inc) {
  int64_t aux = *p;
  yield();
  aux += inc;
  *p = aux;
}

static int my_process_inc(uint64_t argc, char *const *argv) {
  uint64_t n;
  int8_t inc;
  int8_t use_sem;

  if (argc != 4) return -1;

  if ((n = satoi(argv[0])) <= 0) return -1;
  if ((inc = satoi(argv[1])) == 0) return -1;
  if ((use_sem = satoi(argv[2])) < 0) return -1;

  sem_t sem = satoi(argv[3]);

  for (uint64_t i = 0; i < n; i++) {
    if (use_sem) sem_wait(sem);
    slow_inc(&global, inc);
    if (use_sem) sem_post(sem);
  }

  return 0;
}

static int test_sync(uint64_t argc, char *const *argv) {
  pid_t pids[2 * TOTAL_PAIR_PROCESSES];

  if (argc != 2) return -1;

  char *argv_dec[] = {argv[0], "-1", argv[1], NULL};
  char *argv_inc[] = {argv[0], "1", argv[1], NULL};

  global = 0;

  sem_t sem = -1;
  uint64_t use_sem = satoi(argv[1]);
  if (use_sem) sem = sem_create(1);

  char sem_str[16];
  char use_sem_str[4];

  uint64_t sem_val = (uint64_t) sem;
  uint64_t i = 0;
  do {
    sem_str[i++] = '0' + (sem_val % 10);
    sem_val /= 10;
  } while (sem_val > 0);
  sem_str[i] = '\0';
  for (uint64_t j = 0; j < i / 2; j++) {
    char tmp = sem_str[j];
    sem_str[j] = sem_str[i - j - 1];
    sem_str[i - j - 1] = tmp;
  }

  strcpy(use_sem_str, argv[1]);

  char *argv_dec_full[] = {"my_process_inc", argv_dec[0], argv_dec[1], use_sem_str, sem_str};
  char *argv_inc_full[] = {"my_process_inc", argv_inc[0], argv_inc[1], use_sem_str, sem_str};

  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    pids[i] = proc_spawn(my_process_inc, 4, &argv_dec_full[1], NULL);
    pids[i + TOTAL_PAIR_PROCESSES] =
      proc_spawn(my_process_inc, 4, &argv_inc_full[1], NULL);
  }

  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    proc_wait(pids[i]);
    proc_wait(pids[i + TOTAL_PAIR_PROCESSES]);
  }

  printf("Final value: %lld\n", global);

  if (use_sem) sem_close(sem);

  return 0;
}

typedef enum { RUNNING, BLOCKED, KILLED } proc_test_state_t;

typedef struct {
  pid_t pid;
  proc_test_state_t state;
} proc_rq_t;

static int endless_loop_wrapper(uint64_t argc, char *const *argv) {
  endless_loop();
  return 0;
}

static int test_processes(uint64_t argc, char *const *argv) {
  uint8_t rq;
  uint8_t alive = 0;
  uint8_t action;
  uint64_t max_processes;
  char *argv_aux[] = {"endless_loop"};

  if (argc != 1) return -1;

  if ((max_processes = satoi(argv[0])) <= 0) return -1;

  proc_rq_t p_rqs[max_processes];

  while (1) {
    // Create max_processes processes
    for (rq = 0; rq < max_processes; rq++) {
      p_rqs[rq].pid = proc_spawn(endless_loop_wrapper, 1, argv_aux, NULL);

      if (p_rqs[rq].pid == -1) {
        printf("test_processes: ERROR creating process\n");
        return -1;
      } else {
        p_rqs[rq].state = RUNNING;
        alive++;
      }
    }

    // Randomly kills, blocks or unblocks processes until every one has been
    // killed
    while (alive > 0) {
      for (rq = 0; rq < max_processes; rq++) {
        action = get_uniform(100) % 2;

        switch (action) {
        case 0:
          if (p_rqs[rq].state == RUNNING || p_rqs[rq].state == BLOCKED) {
            proc_kill(p_rqs[rq].pid);
            p_rqs[rq].state = KILLED;
            alive--;
          }
          break;

        case 1:
          if (p_rqs[rq].state == RUNNING) {
            proc_block(p_rqs[rq].pid);
            p_rqs[rq].state = BLOCKED;
          }
          break;
        }
      }

      // Randomly unblocks processes
      for (rq = 0; rq < max_processes; rq++)
        if (p_rqs[rq].state == BLOCKED && get_uniform(100) % 2) {
          proc_run(p_rqs[rq].pid);
          p_rqs[rq].state = RUNNING;
        }
    }
  }
}

typedef struct {
  const char *name;
  const char *desc;
} test_t;

static test_t tests[] = {
  {"mm", "Memory allocator test"},
  {"sync", "Synchronization test with semaphores"},
  {"nosync", "Synchronization test without semaphores"},
  {"processes", "Process creation, blocking and killing test"},
};
static size_t n_tests = sizeof(tests) / sizeof(test_t);

static int print_help() {
  printf("Available tests:\n\n");

  for (int i = 0; i < n_tests; i++) {
    printf(
      COL_BLUE "%s" COL_RESET " - %s\n", tests[i].name, tests[i].desc
    );
  }

  return 0;
}

int test(uint64_t argc, char *const *argv) {
  if (argc < 2) {
    printf("Usage: test <subcommand>\n");
    printf("Type " COL_YELLOW "test help" COL_RESET " for a list of tests\n");
    return 0;
  }

  if (!strcmp(argv[1], "help")) { return print_help(); }

  if (!strcmp(argv[1], "mm")) {
    if (argc < 3) {
      printf(COL_RED "test mm: missing max_memory argument\n");
      return 1;
    }
    return test_mm(1, &argv[2]);
  }

  if (!strcmp(argv[1], "sync")) {
    if (argc < 3) {
      printf(COL_RED "test sync: missing arguments\n");
      printf("Usage: test sync <n> <use_sem>\n");
      return 1;
    }
    char *sync_argv[] = {argv[2], "1"};
    return test_sync(2, sync_argv);
  }

  if (!strcmp(argv[1], "nosync")) {
    if (argc < 3) {
      printf(COL_RED "test nosync: missing arguments\n");
      printf("Usage: test nosync <n>\n");
      return 1;
    }
    char *nosync_argv[] = {argv[2], "0"};
    return test_sync(2, nosync_argv);
  }

  if (!strcmp(argv[1], "processes")) {
    if (argc < 3) {
      printf(COL_RED "test processes: missing max_processes argument\n");
      return 1;
    }
    return test_processes(1, &argv[2]);
  }

  printf(COL_RED "test: invalid subcommand %s\n", argv[1]);
  printf("Type " COL_YELLOW "test help" COL_RESET " for a list of tests\n");
  return 1;
}
