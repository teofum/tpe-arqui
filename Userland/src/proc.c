#include <print.h>
#include <proc.h>
#include <process.h>
#include <shell.h>
#include <strings.h>

typedef struct {
  const char *cmd;
  const char *desc;
} command_t;

static char *const state_str[] = {
  [PROC_STATE_RUNNING] = COL_GREEN "Running" COL_RESET,
  [PROC_STATE_BLOCKED] = COL_YELLOW "Blocked" COL_RESET,
  [PROC_STATE_EXITED] = COL_GRAY "Exited " COL_RESET,
};

static int print_help();
static int ps();
static int make_foreground(pid_t pid);
static int kill(pid_t pid);
static command_t commands[] = {
  {"help", "Display this help message"},
  {"ls", "List all running processes"},
  {"fg", "Bring a process into foreground"},
  {"kill", "Kill a process"},
};
static size_t n_commands = sizeof(commands) / sizeof(command_t);

static int print_help() {
  printf("Available commands:\n\n");

  for (int i = 0; i < n_commands; i++) {
    printf(
      COL_BLUE "%s" COL_RESET "\t- %s\n", commands[i].cmd, commands[i].desc
    );
  }

  return 0;
}

static int ps() {
  printf(
    COL_GRAY
    "PID  Name             State    Priority Foreground RSP               \n"
  );
  printf(
    COL_GRAY
    "-----------------------------------------------------------------------\n"
  );

  proc_info_t info;
  for (pid_t i = 0; i <= MAX_PID; i++) {
    if (proc_info(i, &info)) {
      printf(
        COL_MAGENTA "%4u " COL_RESET "%16s %s " COL_MAGENTA "%9u " COL_BLUE
                    "%10s " COL_MAGENTA "%#016llx\n",
        info.pid, info.description, state_str[info.state], info.priority,
        info.foreground ? "Yes" : " ", info.rsp
      );
    }
  }

  return 0;
}

static int make_foreground(pid_t pid) {
  if (pid < 2) {
    printf(COL_RED "Cannot bring a system process to foreground\n");
    return 1;
  }
  if (pid == 2) {
    printf(COL_RED "Cannot bring the shell process to foreground\n");
    return 1;
  }

  // TODO fail if pid does not exist

  proc_wait_for_foreground();
  return proc_wait(pid);
}

static int kill(pid_t pid) {
  if (pid < 2) {
    printf(COL_RED "Cannot kill a system process\n");
    return 1;
  }
  if (pid == 2) {
    printf(COL_RED "Cannot kill the shell process\n");
    return 1;
  }

  // TODO fail if pid does not exist

  proc_kill(pid);
  proc_wait_for_foreground();
  proc_wait(pid);
  return 0;
}

static uint32_t parse_uint(const char *s) {
  uint32_t r = 0;
  while (*s >= '0' && *s <= '9') {
    r *= 10;
    r += *s - '0';
    s++;
  }
  return r;
}

int proc(uint64_t argc, char *const *argv) {
  if (argc < 2) {
    printf("Usage: proc <command>\n");
    printf("Type " COL_YELLOW "proc help" COL_RESET " for a list of commands\n"
    );
    return 0;
  }

  if (!strcmp(argv[1], "help")) { return print_help(); }
  if (!strcmp(argv[1], "ls")) { return ps(); }

  if (argc < 3) {
    printf(COL_RED "proc %s: missing pid\n", argv[1]);
    return 1;
  }
  pid_t pid = parse_uint(argv[2]);

  if (!strcmp(argv[1], "fg")) { return make_foreground(pid); }
  if (!strcmp(argv[1], "kill")) { return kill(pid); }

  printf(COL_RED "proc: invalid command %s\n", argv[1]);
  printf("Type " COL_YELLOW "proc help" COL_RESET " for a list of commands\n");
  return 1;
}
