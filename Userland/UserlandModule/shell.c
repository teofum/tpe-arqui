#include <print.h>
#include <shell.h>
#include <strings.h>
#include <syscall.h>

#include <gfxdemo.h>

#define CMD_BUF_LEN 256
#define HISTORY_SIZE 64

typedef enum {
  RET_EXIT = -1,
  RET_UNKNOWN_CMD = -255,
} retcode_t;

typedef struct {
  const char *cmd;
  const char *desc;
  int (*entryPoint)(const char *args);
} command_t;

char commandHistory[HISTORY_SIZE][CMD_BUF_LEN];
uint32_t historyPointer = 0;
uint32_t promptLength = 2;

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
  if (name == NULL) {
    printf(
      COL_RED "Missing font name\n" COL_RESET "Usage: setfont <font name>\n"
              "Hint: Type " COL_YELLOW "'setfont ls'" COL_RESET
              " for a list of fonts\n"
    );
    return 1;
  }

  if (strcmp(name, "ls") == 0) {
    for (int i = 0; i < nFonts; i++) { printf(COL_BLUE "%s\n", fonts[i].name); }
    return 0;
  }

  for (int i = 0; i < nFonts; i++) {
    if (strcmp(name, fonts[i].name) == 0) {
      _syscall(SYS_FONT, fonts[i].id);
      return 0;
    }
  }

  printf(
    COL_RED "Unknown font name '%s'\n" COL_RESET "Hint: Type " COL_YELLOW
            "'setfont ls'" COL_RESET " for a list of fonts\n",
    name
  );

  return 2;
}

static int history() {
  for (int i = 0; i < historyPointer; i++) {
    printf("%s\n", commandHistory[i]);
  }
  return 0;
}

static int status(const char *param) {
  if (param == NULL) {
    printf("Usage: status <on/off>\n");
    return 1;
  }

  if (!strcmp(param, "off")) {
    _syscall(SYS_STATUS_SET_ENABLED, 0);
    _syscall(SYS_CLEAR);
    return 0;
  } else if (!strcmp(param, "on")) {
    _syscall(SYS_STATUS_SET_ENABLED, 1);
    _syscall(SYS_CLEAR);
    return 0;
  }

  printf(
    COL_RED "Invalid argument '%s'\n" COL_RESET "Usage: status <on/off>\n",
    param
  );

  return 2;
}

extern void _throw_00();
int throw00(){
  _throw_00();
  return 0;
}

extern void _throw_06();
int throw06(){
  _throw_06();
  return 0;
}

static int help();
command_t commands[] = {
  {"help", "Display this help message", help},
  {"echo", "Print arguments to stdout", echo},
  {"exit", "Exit the shell and return to kernel", exit},
  {"clear", "Clear stdout", clear},
  {"setfont", "Set text mode font", setfont},
  {"gfxdemo", "Graphics mode demo", gfxdemo},
  {"history", "Print command history", history},
  {"status", "Turn the system status bar on or off", status},
  {"exc00", "Tests Division by Zero Exception", throw00},
  {"exc06", "Test OpCode Exception", throw06},
};
size_t nCommands = sizeof(commands) / sizeof(command_t);

static int help() {
  printf(
    "Welcome to " COL_GREEN "tpeOS" COL_RESET "!\n"
    "Available commands:\n\n"
  );

  for (int i = 0; i < nCommands; i++) {
    printf(
      COL_BLUE "%s" COL_RESET "\t- %s\n", commands[i].cmd, commands[i].desc
    );
  }

  printf(
    "\nPress the " COL_YELLOW "F1" COL_RESET " key any time to dump CPU state\n"
  );
  return 0;
}

static void writePrompt() { printf("> "); }

static void readCommand(char *cmd) {
  int inputEnd = 0;
  uint32_t localHistoryPointer = historyPointer;

  uint32_t cmdWritePtr = 0;
  uint32_t back = 0;
  uint32_t backspaces = 0;
  char *cmdStart = cmd;
  char temp[CMD_BUF_LEN] = {0};

  while (!inputEnd) {
    uint32_t initialPtr = cmdWritePtr;

    // Wait for input on stdin
    int len;
    do { len = _syscall(SYS_READ, temp, CMD_BUF_LEN); } while (!len);

    for (int i = 0; i < len; i++) {
      if (temp[i] == '\n') {
        inputEnd = 1;
        break;
      } else if (temp[i] == 0x1B) {
        if (temp[i + 1] == '[') {
          switch (temp[i + 2]) {
            case 'A':
              // Up
              if (localHistoryPointer > 0) {
                char *last = commandHistory[--localHistoryPointer];
                _syscall(SYS_BLANKLINE, promptLength);
                cmd = cmdStart;
                cmdWritePtr = strcpy(cmd, last);
                initialPtr = 0;
              }
              break;
            case 'B':
              // Down
              if (localHistoryPointer < historyPointer - 1) {
                char *last = commandHistory[++localHistoryPointer];
                _syscall(SYS_BLANKLINE, promptLength);
                cmd = cmdStart;
                cmdWritePtr = strcpy(cmd, last);
                initialPtr = 0;
              }
              break;
            case 'C':
              // Right
              if (back > 0) {
                cmdWritePtr++;
                initialPtr++;
                back--;
                _syscall(
                  SYS_CURSOR, back == 0 ? IO_CURSOR_UNDER : IO_CURSOR_BLOCK
                );
                _syscall(SYS_CURMOVE, 1);
              }
              break;
            case 'D':
              // Left
              if (cmdWritePtr > 0) {
                cmdWritePtr--;
                back++;
                _syscall(SYS_CURSOR, IO_CURSOR_BLOCK);
                _syscall(SYS_CURMOVE, -1);
              }
              break;
          }
        }
        i += 2;
      } else if (temp[i] == '\b') {
        // Discard backspaces if we're not at the end of the command, or if half
        // the typed chars are backspaces (prevents us from deleting the prompt)
        if (back == 0 && initialPtr > backspaces * 2) {
          cmd[cmdWritePtr++] = temp[i];
          backspaces++;
        }
      } else {
        cmd[cmdWritePtr++] = temp[i];
        if (back > 0) {
          back--;
          _syscall(SYS_CURSOR, back == 0 ? IO_CURSOR_UNDER : IO_CURSOR_BLOCK);
        }
      }
    }

    if (cmdWritePtr > initialPtr) {
      _syscall(SYS_WRITE, cmd + initialPtr, cmdWritePtr - initialPtr);
    }
  }

  cmd[cmdWritePtr + back] = 0;
  _syscall(SYS_PUTC, '\n');
  _syscall(SYS_CURSOR, IO_CURSOR_UNDER);

  // Handle backspaces
  cmd = cmdStart;
  int j = 0;
  for (int i = 0; cmd[i]; i++) {
    if (cmd[i] == '\b') {
      if (j > 0) j--;
    } else {
      cmd[j++] = cmd[i];
    }
  }
  cmd[j] = 0;

  if (historyPointer == 0 ||
      strcmp(commandHistory[historyPointer - 1], cmd) != 0) {
    if (historyPointer == HISTORY_SIZE) {
      memcpy(
        commandHistory[0], commandHistory[1], (HISTORY_SIZE - 1) * CMD_BUF_LEN
      );
      historyPointer--;
    }

    strcpy(commandHistory[historyPointer++], cmd);
  }
}

static int runCommand(const char *cmd) {
  char cmdName[CMD_BUF_LEN];
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
      COL_RED "Unknown command '%s'\n" COL_RESET "Hint: Type " COL_YELLOW
              "'help'" COL_RESET " for a list of available commands\n",
      cmdName
    );
  } else if (retcode == RET_EXIT) {
    return 1;
  } else if (retcode != 0) {
    promptLength = 2 + printf("[" COL_RED "%u" COL_RESET "] ", retcode);
  } else {
    promptLength = 2;
  }


  return 0;
}

int startShell() {
  char cmdBuf[CMD_BUF_LEN];

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

