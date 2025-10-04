#include <mem.h>
#include <print.h>
#include <process.h>
#include <shell.h>
#include <sst.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>

#define TEST_PASS "[" COL_GREEN "PASS" COL_RESET "] "
#define TEST_FAIL "[" COL_RED "FAIL" COL_RESET "] "

#define lengthof(x) (sizeof((x)) / sizeof((x)[0]))

/*
 * Test function, should take no arguments and return 0 on success, 1 on failure.
 */
typedef int (*test_fn_t)();

/* ========================================================================= *
 * Tests start here                                                          *
 * ========================================================================= */

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
  printf("    Memory allocation must return a valid address\n");

  void *mem = mem_alloc(1024);
  printf("    Got 1024 bytes at %#016lx\n", (size_t) mem);

  // Hardcoded heap memory start address, guaranteed to clear kernel and userland data
  if ((size_t) mem < 0x1000000) {
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
  printf("    Memory allocation must be exclusive\n");

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

/*
 * Test memory free and check functionality
 */
int test_mem_free() {
  printf("    Memory free and check must work correctly\n");

  void *mem1 = mem_alloc(128);
  void *mem2 = mem_alloc(64);
  printf("    Allocated mem1=%#016lx mem2=%#016lx\n", (size_t) mem1, (size_t) mem2);

  printf(
    "    Before free: check(mem1)=%u check(mem2)=%u\n", mem_check(mem1),
    mem_check(mem2)
  );

  if (!mem_check(mem1) || !mem_check(mem2)) {
    printf(COL_RED "    Allocated memory should be marked as allocated\n" COL_RESET);
    return 1;
  }

  mem_free(mem1);
  printf(
    "    After free mem1: check(mem1)=%u check(mem2)=%u\n", mem_check(mem1),
    mem_check(mem2)
  );

  if (mem_check(mem1) || !mem_check(mem2)) {
    printf(COL_RED "    mem1 should be free, mem2 should still be allocated\n" COL_RESET);
    return 1;
  }

  mem_free(mem2);
  printf(
    "    After free mem2: check(mem1)=%u check(mem2)=%u\n", mem_check(mem1),
    mem_check(mem2)
  );

  if (mem_check(mem1) || mem_check(mem2)) {
    printf(COL_RED "    Both blocks should be free\n" COL_RESET);
    return 1;
  }

  return 0;
}

/*
 * Test a process spawns, exits and returns correctly
 */
static int dummy_process() { return 42; }
int test_proc_spawn_wait() {
  printf("    Process must start and exit successfully\n");

  char *const argv[1] = {"test"};
  pid_t pid = proc_spawn(dummy_process, 1, argv);
  int return_code = proc_wait(pid);

  printf("    Process with pid %u exited with code %u\n", pid, return_code);

  if (return_code != 42) {
    printf(COL_RED "    Bad return code: expected 42, got %u\n", return_code);
    return 1;
  }

  return 0;
}

/*
 * Test a process spawns, exits and returns correctly
 */
static int args_process(uint64_t argc, char *const *argv) {
  printf("    Child: Received " COL_MAGENTA "%llu" COL_RESET " args (", argc);
  for (uint64_t i = 0; i < argc; i++)
    printf(
      COL_MAGENTA "'%s'" COL_RESET "%s", argv[i], i == argc - 1 ? "" : ", "
    );
  printf(")\n");

  if (argc != 3) {
    printf(COL_RED "    Bad arg count: expected 3, got %llu\n", argc);
    return 1;
  }

  if (strcmp(argv[0], "args_process") != 0) {
    printf(
      COL_RED "    Bad argv[0]: expected 'args_process', got '%s'\n", argv[0]
    );
    return 1;
  }
  if (strcmp(argv[1], "foo") != 0) {
    printf(COL_RED "    Bad argv[1]: expected 'foo', got '%s'\n", argv[1]);
    return 1;
  }
  if (strcmp(argv[2], "bar") != 0) {
    printf(COL_RED "    Bad argv[2]: expected 'var', got '%s'\n", argv[2]);
    return 1;
  }

  return 0;
}
int test_proc_args() {
  printf("    Process must receive arguments as sent\n");

  char *const argv[] = {"args_process", "foo", "bar"};
  pid_t pid = proc_spawn(args_process, lengthof(argv), argv);
  int return_code = proc_wait(pid);

  printf("    Process with pid %u exited with code %u\n", pid, return_code);

  return return_code;
}

/* ========================================================================= *
 * Tests end here                                                            *
 * ========================================================================= */

test_fn_t tests[] = {
  test_sanity_check, test_mem_alloc, test_mem_exclusive, test_mem_free, test_proc_spawn_wait,
  test_proc_args
};

int sst_run_tests() {
  int result = 0;

  int n_tests = sizeof(tests) / sizeof(test_fn_t);

  printf("[" COL_BLUE "SST" COL_RESET "] Running Startup Self-Test suite...\n");

  for (int i = 0; i < n_tests; i++) {
    printf(
      "[" COL_BLUE "SST" COL_RESET "] Running test %u of %u\n", i + 1, n_tests
    );
    int test_result = tests[i]();

    printf(
      "%sTest %u of %u\n", test_result ? TEST_FAIL : TEST_PASS, i + 1, n_tests
    );
    result += test_result;
  }

  return result;
}
