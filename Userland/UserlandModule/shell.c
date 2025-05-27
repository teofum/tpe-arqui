#include <print.h>
#include <shell.h>
#include <strings.h>
#include <syscall.h>

typedef enum {
  RET_EXIT = -1,
  RET_UNKNOWN_CMD = -255,
} retcode_t;

typedef struct {
  const char *cmd;
  int (*entryPoint)(const char *args);
} command_t;

static int echo(const char *args) {
  printf("%s\n", args != NULL ? args : "");

  return 0;
}

static int exit() { return RET_EXIT; }

static int clear() {
  _syscall(SYS_CLEAR);
  return 0;
}

typedef struct {
  const char *name;
  io_font_t id;
} font_entry_t;

font_entry_t fonts[] = {
  {"default", IO_FONT_DEFAULT},
  {"tiny", IO_FONT_TINY},
  {"tiny bold", IO_FONT_TINY_BOLD},
  {"small", IO_FONT_SMALL},
  {"large", IO_FONT_LARGE},
  {"alt", IO_FONT_ALT},
  {"alt bold", IO_FONT_ALT_BOLD},
  {"future", IO_FONT_FUTURE},
  {"old", IO_FONT_OLD},
};
size_t nFonts = sizeof(fonts) / sizeof(font_entry_t);

static int setfont(const char *name) {
  for (int i = 0; i < nFonts; i++) {
    if (strcmp(name, fonts[i].name) == 0) {
      _syscall(SYS_FONT, fonts[i].id);
      return 0;
    }
  }

  return 1;
}

command_t commands[] = {
  {"echo", echo},
  {"exit", exit},
  {"clear", clear},
  {"setfont", setfont},
};
size_t nCommands = sizeof(commands) / sizeof(command_t);

static void writePrompt() { printf("> "); }

static void readCommand(char *buf) {
  int inputEnd = 0;
  char *start = buf;

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

  // Handle backspaces, etc
  buf = start;
  int j = 0;
  for (int i = 0; buf[i]; i++) {
    if (buf[i] == '\b') {
      if (j > 0) j--;
    } else {
      buf[j++] = buf[i];
    }
  }
  buf[j] = 0;
}

static int runCommand(const char *cmd) {
  char cmdName[256];
  cmd = strsplit(cmdName, cmd, ' ');

  if (cmdName[0] == 0) return 0;

  // Linear search all commands. Not super efficient, but the number of
  // commands is quite small so we don't need to worry too much.
  int retcode = RET_UNKNOWN_CMD;
  for (int i = 0; i < nCommands; i++) {
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
  command_t cmd_clear = {
    .cmd = "clear",
    .entryPoint = clear,
  };
  commands[2] = cmd_clear;

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
