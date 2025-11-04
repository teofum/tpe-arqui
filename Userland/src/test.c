#include <mem.h>
#include <print.h>
#include <process.h>
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
      mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
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

static int print_help() {
  printf("Available tests:\n\n");
  printf(COL_BLUE "mm" COL_RESET " <max_memory>\t- Memory allocator stress test\n");
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

  printf(COL_RED "test: invalid subcommand %s\n", argv[1]);
  printf("Type " COL_YELLOW "test help" COL_RESET " for a list of tests\n");
  return 1;
}
