#include "fd.h"
#include <io.h>
#include <mem.h>
#include <print.h>
#include <process.h>
#include <scheduler.h>
#include <semaphores.h>
#include <shell.h>
#include <sst.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>

#define TEST_PASS "[" COL_GREEN "PASS" COL_RESET "] "
#define TEST_FAIL "[" COL_RED "FAIL" COL_RESET "] "

#define sst_assert(condition, printf_args...)                                  \
  if (!(condition)) {                                                          \
    printf(COL_RED "    " printf_args);                                        \
    return 1;                                                                  \
  }

#define sst_assert_equal(expected, actual, fail_msg)                           \
  if ((expected) != (actual)) {                                                \
    printf(                                                                    \
      COL_RED "    %s: Expected %llu, got %llu\n", fail_msg,                   \
      (uint64_t) expected, (uint64_t) actual                                   \
    );                                                                         \
    return 1;                                                                  \
  }

#define sst_assert_streq(expected, actual, fail_msg)                           \
  if (strcmp((expected), (actual))) {                                          \
    printf(                                                                    \
      COL_RED "    %s: Expected '%s', got '%s'\n", fail_msg, expected, actual  \
    );                                                                         \
    return 1;                                                                  \
  }

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
  sst_assert(
    (size_t) mem >= 0x1000000,
    "Address %#016lx is in reserved memory zone (0x0 - 0x3FFFFFF)\n",
    (size_t) mem
  );

  ((int *) mem)[1023] = 42;
  sst_assert_equal(
    42, ((int *) mem)[1023],
    "Failed to read back written value from allocated memory"
  );

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

    if (i > 0) {
      sst_assert(
        (size_t) mem[i] >= (size_t) mem[i - 1] + last_size,
        "Allocated memory overlap at %#016lx\n", (size_t) mem[i]
      );
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

  printf(
    "    Allocated mem1=%#016lx mem2=%#016lx\n", (size_t) mem1, (size_t) mem2
  );

  printf(
    "    Before free: check(mem1)=%u check(mem2)=%u\n", mem_check(mem1),
    mem_check(mem2)
  );

  sst_assert(
    mem_check(mem1) && mem_check(mem2),
    "Allocated memory should be marked as allocated\n"
  );

  mem_free(mem1);
  printf(
    "    After free mem1: check(mem1)=%u check(mem2)=%u\n", mem_check(mem1),
    mem_check(mem2)
  );

  sst_assert(
    !mem_check(mem1) && mem_check(mem2),
    "mem1 should be free, mem2 should still be allocated\n"
  );

  mem_free(mem2);
  printf(
    "    After free mem2: check(mem1)=%u check(mem2)=%u\n", mem_check(mem1),
    mem_check(mem2)
  );

  sst_assert(
    !mem_check(mem1) && !mem_check(mem2), "Both blocks should be free\n"
  );

  return 0;
}

/*
 * Test a process spawns, exits and returns correctly
 */
static int dummy_process() { return 42; }
int test_proc_spawn_wait() {
  printf("    Process must start and exit successfully\n");

  char *const argv[1] = {"test"};
  pid_t pid = proc_spawn(dummy_process, 1, argv, NULL);
  int return_code = proc_wait(pid);

  printf("    Process with pid %u exited with code %u\n", pid, return_code);
  sst_assert_equal(42, return_code, "Bad return code");

  return 0;
}

/*
 * Test getpid() works as expected
 */
static int pid_process() {
  pid_t my_pid = getpid();
  printf("    Child: my PID is " COL_MAGENTA "%u\n", (uint32_t) my_pid);
  return my_pid;
}
int test_proc_getpid() {
  printf("    getpid() must return the running process PID\n");

  char *const argv[1] = {"pid_process"};
  pid_t pid = proc_spawn(pid_process, 1, argv, NULL);
  int return_code = proc_wait(pid);

  printf("    Process with pid %u exited with code %u\n", pid, return_code);
  sst_assert_equal(pid, return_code, "getpid() does not match actual PID\n");

  return 0;
}

/*
 * Test a process receives its arguments correctly
 */
static int args_process(uint64_t argc, char *const *argv) {
  printf("    Child: Received " COL_MAGENTA "%llu" COL_RESET " args (", argc);
  for (uint64_t i = 0; i < argc; i++)
    printf(
      COL_MAGENTA "'%s'" COL_RESET "%s", argv[i], i == argc - 1 ? "" : ", "
    );
  printf(")\n");

  sst_assert_equal(3, argc, "Bad arg count");
  sst_assert_streq("args_process", argv[0], "Bad argv[0]");
  sst_assert_streq("foo", argv[1], "Bad argv[1]");
  sst_assert_streq("bar", argv[2], "Bad argv[2]");

  return 0;
}
int test_proc_args() {
  printf("    Process must receive arguments as sent\n");

  char *const argv[] = {"args_process", "foo", "bar"};
  pid_t pid = proc_spawn(args_process, lengthof(argv), argv, NULL);
  int return_code = proc_wait(pid);

  printf("    Process with pid %u exited with code %u\n", pid, return_code);

  return return_code;
}

/*
 * Test a process cannot modify its args
 */
static int evil(uint64_t argc, char *const *argv) {
  printf("    Child: Replacing first arg...\n");
  char **a = (char **) &argv[1];
  *a = "hax";

  return 0;
}
static int evil2(uint64_t argc, char *const *argv) {
  printf("    Child: Overwriting first arg...\n");
  strcpy(argv[1], "hax");

  return 0;
}
int test_proc_args_copy() {
  char *const argv[] = {"args_process", mem_alloc(4)};
  strcpy(argv[1], "foo");

  printf("    Process must not be able to replace its arguments\n");
  printf("    Spawning process with argv[1] = 'foo'\n");
  pid_t pid = proc_spawn(evil, lengthof(argv), argv, NULL);
  proc_wait(pid);
  sst_assert_streq("foo", argv[1], "Process replaced an argument");

  printf("    Process must not be able to modify its arguments\n");
  printf("    Spawning process with argv[1] = 'foo'\n");
  pid = proc_spawn(evil2, lengthof(argv), argv, NULL);
  proc_wait(pid);
  sst_assert_streq("foo", argv[1], "Process modified an argument");

  return 0;
}

/*
 * Test a process cannot read from an unset fd
 */
static int reader(uint64_t argc, char *const *argv) {
  printf("    Child: Reading from FD 42...\n");
  return read(42, NULL, 1);
}
int test_proc_read_unset_fd() {
  char *const argv[] = {"reader"};

  printf("    Process must not be able to read from an unset FD\n");
  pid_t pid = proc_spawn(reader, lengthof(argv), argv, NULL);
  int ret = proc_wait(pid);
  sst_assert_equal(-1, ret, "Process succesfully read from unset FD 42");

  return 0;
}

/*
 * Test a process cannot write from an unset fd
 */
static int writer(uint64_t argc, char *const *argv) {
  printf("    Child: Writing to FD 42...\n");
  return write(42, argv[0], 6);
}
int test_proc_write_unset_fd() {
  char *const argv[] = {"writer"};

  printf("    Process must not be able to write to an unset FD\n");
  pid_t pid = proc_spawn(writer, lengthof(argv), argv, NULL);
  int ret = proc_wait(pid);
  sst_assert_equal(-1, ret, "Process succesfully wrote to unset FD 42");

  return 0;
}

int test_sem_ops() {
  printf("    Semaphore operations must work correctly\n");

  sem_t sem = sem_create(0);
  sst_assert(sem != -1, "Failed to create semaphore\n");

  int result = sem_post(sem);
  sst_assert_equal(0, result, "sem_post failed");
  printf("flag\n");
  result = sem_wait(sem);
  sst_assert_equal(0, result, "sem_wait failed");

  sem_close(sem);
  return 0;
}


sem_t global_sem;
sem_t global_sem2;
int check = 0;

static int sem_post_test(uint64_t argc, char *const *argv) {
  sem_wait(global_sem2);
  check = 2;
  sem_post(global_sem);
  return 0;
}

int test_sem_sync() {
  printf("    Semaphore must synchronize between processes\n");

  global_sem = sem_create(0);
  global_sem2 = sem_create(0);

  pid_t pid = proc_spawn(sem_post_test, 0, NULL, NULL);

  check = 1;
  sem_post(global_sem2);
  sem_wait(global_sem);

  sst_assert_equal(2, check, "sem sync failed");

  proc_wait(pid);
  sem_close(global_sem);
  sem_close(global_sem2);
  return 0;
}

/*
 * Test named pipes
 */
static int pipe_writer(uint64_t argc, char *const *argv) {
  open(5, 2, FD_WRITE);
  printf("    Writer: writing \"%s\" to pipe\n", argv[1]);
  write(5, argv[1], 4);
  close(5);
  return 0;
}

static int pipe_reader(uint64_t argc, char *const *argv) {
  char buf[8] = {0};

  open(7, 2, FD_READ);
  read(7, buf, 4);
  close(7);

  printf("    Reader: read \"%s\" from pipe\n", argv[1]);
  sst_assert_streq(argv[1], buf, "String read from pipe doesn't match");

  return 0;
}

int test_named_pipe() {
  pipe_create_named(2);

  char *const argv[2] = {"", "test"};
  pid_t writer_pid = proc_spawn(pipe_writer, lengthof(argv), argv, NULL);
  pid_t reader_pid = proc_spawn(pipe_reader, lengthof(argv), argv, NULL);

  proc_wait(writer_pid);
  proc_wait(reader_pid);

  return 0;
}

/* ========================================================================= *
 * Tests end here                                                            *
 * ========================================================================= */

test_fn_t tests[] = {
  test_sanity_check,
  test_mem_alloc,
  test_mem_exclusive,
  test_mem_free,
  test_proc_spawn_wait,
  test_proc_getpid,
  test_proc_args,
  test_proc_args_copy,
  test_proc_read_unset_fd,
  test_proc_write_unset_fd,
  test_sem_ops,
  test_sem_sync,
  test_named_pipe,
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
