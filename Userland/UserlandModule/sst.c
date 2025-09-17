#include <mem.h>
#include <print.h>
#include <shell.h>
#include <sst.h>

#define TEST_PASS "[" COL_GREEN "PASS" COL_RESET "] "
#define TEST_FAIL "[" COL_RED "FAIL" COL_RESET "] "

/*
 * Test function, should take no arguments and return 0 on success, 1 on failure.
 */
typedef int (*test_fn_t)();

/*
 * Sanity check, should always run and pass
 */
int test_sanity_check() {
  printf("    Sanity check: this test should run and pass\n");
  return 0;
}

/*
 * Test the memory allocator returns an address clear of kernel and userland
 * code, and the returned address can be written to and read back from.
 */
int test_mem_alloc() {
  printf("    Memory allocation should return a valid address\n");

  void *mem = mem_alloc(1024);
  printf("    Got 1024 bytes at %#016lx\n", (size_t) mem);

  // Hardcoded heap memory start address, guaranteed to clear kernel and userland data
  if ((size_t) mem < 0x4000000) {
    printf(
      COL_RED "    Address %#016lx is in reserved memory zone (0x0 - "
              "0x3FFFFFF)\n" COL_RESET,
      (size_t) mem
    );
    return 1;
  }

  ((int *) mem)[1023] = 42;
  if (((int *) mem)[1023] != 42) {
    printf(
      COL_RED
      "    Failed to read back written value from allocated memory\n" COL_RESET
    );
    return 1;
  }

  mem_free(mem);
  return 0;
}

/*
 * Test the memory allocator gives exclusive access to memory
 */
int test_mem_exclusive() {
  printf("    Memory allocation should be exclusive\n");

  size_t size = 1024, last_size = 0;
  void *mem[4];
  for (int i = 0; i < 4; i++) {
    mem[i] = mem_alloc(size);
    printf("    Got %lu bytes at %#016lx\n", size, (size_t) mem[i]);

    if (i > 0 && ((size_t) mem[i] < (size_t) mem[i - 1] + last_size)) {
      printf(
        COL_RED "    Allocated memory overlap at %#016lx\n" COL_RESET,
        (size_t) mem[i]
      );
      return 1;
    }

    last_size = size;
    size *= 2;
  }

  for (int i = 0; i < 4; i++) mem_free(mem[i]);
  return 0;
}

test_fn_t tests[] = {test_sanity_check, test_mem_alloc, test_mem_exclusive};

int sst_run_tests() {
  int result = 0;

  int n_tests = sizeof(tests) / sizeof(test_fn_t);

  printf("[" COL_BLUE "SST" COL_RESET "] Running Startup Self-Test suite...\n");

  for (int i = 0; i < n_tests; i++) {
    printf("[" COL_BLUE "SST" COL_RESET "] Test %u of %u\n", i + 1, n_tests);
    int test_result = tests[i]();

    printf(
      "%sTest %u of %u\n", test_result ? TEST_FAIL : TEST_PASS, i + 1, n_tests
    );
    result += test_result;
  }

  return result;
}
