#include <print.h>
#include <shell.h>
#include <strings.h>
#include <syscall.h>

#define CMDS_SIZE 64

typedef enum {
  RET_EXIT = -1,
  RET_UNKNOWN_CMD = -255,
} retcode_t;

typedef struct {
  const char *cmd;
  int (*entryPoint)(const char *args);
} command_t;

command_t commands[CMDS_SIZE] = {0};

static int echo(const char *args) {
  printf("%s\n", args);

  return 0;
}

static int exit() { return RET_EXIT; }

static void writePrompt() { printf("> "); }

static void readCommand(char *buf) {
  int inputEnd = 0;
  while (!inputEnd) {
    int len = _syscall(SYS_READ, buf);
    for (int i = 0; i < len; i++) {
      if (buf[i] == '\n') {
        len = i;
        inputEnd = 1;
      }
    }

    _syscall(SYS_WRITE, buf, len);
    buf += len;
  }

  *buf = 0;
  _syscall(SYS_PUTC, '\n');
}

static int runCommand(const char *cmd) {
  char cmdName[256];
  cmd = strsplit(cmdName, cmd, ' ');

  // Linear search all commands. Not super efficient, but the number of
  // commands is quite small so we don't need to worry too much.
  int retcode = RET_UNKNOWN_CMD;
  for (int i = 0; i < CMDS_SIZE && commands[i].cmd; i++) {
    if (strcmp(cmdName, commands[i].cmd) == 0) {
      retcode = commands[i].entryPoint(cmd);
      break;
    }
  }
  if (retcode == RET_UNKNOWN_CMD) {
    printf(
      "Unrecognized command '%s'\n"
      "Type 'help' for a list of available commands\n",
      cmdName
    );
  } else if (retcode == RET_EXIT) {
    return 1;
  }


  return 0;
}

int startShell() {
  char cmdBuf[256];

  // Register commands
  command_t cmd_exit = {
    .cmd = "exit",
    .entryPoint = exit,
  };
  commands[0] = cmd_exit;
  command_t cmd_echo = {
    .cmd = "echo",
    .entryPoint = echo,
  };
  commands[1] = cmd_echo;

  // Run the shell
  int exit = 0;
  while (!exit) {
    writePrompt();
    readCommand(cmdBuf);
    exit = runCommand(cmdBuf);
  }

  // Return to kernel
  // This should almost never happen!
  printf("Goodbye!\n");

  return 0;
}
