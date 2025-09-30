#include <gfxdemo.h>
#include <golf_game.h>
#include <io.h>
#include <mem.h>
#include <print.h>
#include <shell.h>
#include <sound.h>
#include <status.h>
#include <stdint.h>
#include <strings.h>

#define CMD_BUF_LEN 64
#define HISTORY_SIZE 64

extern const char *mascot;

typedef enum {
  RET_EXIT = -1,
  RET_UNKNOWN_CMD = -255,
} retcode_t;

typedef struct {
  const char *cmd;
  const char *desc;
  int (*entryPoint)(const char *args);
} command_t;

char command_history[HISTORY_SIZE][CMD_BUF_LEN];
uint32_t history_pointer = 0;
uint32_t prompt_length = 2;

static int echo(const char *args) {
  printf("%s\n", args != NULL ? args : "");

  return 0;
}

static int exit() { return RET_EXIT; }

static int clear() {
  io_clear();
  return 0;
}

typedef struct {
  const char *name;
  vga_font_t id;
} font_entry_t;

font_entry_t fonts[] = {
  {"default", VGA_FONT_DEFAULT},
  {"tiny", VGA_FONT_TINY},
  {"tiny bold", VGA_FONT_TINY_BOLD},
  {"small", VGA_FONT_SMALL},
  {"large", VGA_FONT_LARGE},
  {"alt", VGA_FONT_ALT},
  {"alt bold", VGA_FONT_ALT_BOLD},
  {"future", VGA_FONT_FUTURE},
  {"old", VGA_FONT_OLD},
};
size_t n_fonts = sizeof(fonts) / sizeof(font_entry_t);

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
    for (int i = 0; i < n_fonts; i++) {
      printf(COL_BLUE "%s\n", fonts[i].name);
    }
    return 0;
  }

  for (int i = 0; i < n_fonts; i++) {
    if (strcmp(name, fonts[i].name) == 0) {
      io_setfont(fonts[i].id);
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
  for (int i = 0; i < history_pointer; i++) {
    printf("%s\n", command_history[i]);
  }
  return 0;
}

static int status(const char *param) {
  if (param == NULL) {
    printf("Usage: status <on/off>\n");
    return 1;
  }

  if (!strcmp(param, "off")) {
    status_set_enabled(0);
    io_clear();
    return 0;
  } else if (!strcmp(param, "on")) {
    status_set_enabled(1);
    io_clear();
    return 0;
  }

  printf(
    COL_RED "Invalid argument '%s'\n" COL_RESET "Usage: status <on|off>\n",
    param
  );

  return 2;
}

extern void _throw_00();
extern void _throw_06();
extern void _regdump_test();
int exception_test(const char *param) {
  if (!strcmp(param, "0")) {
    _throw_00();
  } else if (!strcmp(param, "6")) {
    _throw_06();
  } else if (!strcmp(param, "test_regdump")) {
    _regdump_test();
  } else {
    printf(
      COL_RED "Invalid exception type '%s'\n" COL_RESET
              "Usage: except <0|6|test_regdump>\n",
      param
    );
  }

  return 0;
}

static int beep() {
  sound_shell_beep();
  return 0;
}

static int music() {
  sound_play_tetris();
  return 0;
}

static int print_mascot() {
  writes(mascot);
  return 0;
}

static int test_alloc() {
  void *mem = mem_alloc(1024);
  printf("Allocated 1024 bytes at address %#016lx\n", (size_t) mem);
  return 0;
}

static int test_check() {
  void *ptr = mem_alloc(256);
  printf("Allocated 256 bytes at %#016lx\n", (size_t) ptr);
  printf("mem_check(allocated ptr): %u (should be 1)\n", mem_check(ptr));
  printf("mem_check(random address): %u (should be 0)\n", mem_check((void*)0x12345));
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
  {"demo3d", "3d Graphics demo", demo3d},
  {"history", "Print command history", history},
  {"status", "Turn the system status bar on or off", status},
  {"beep", "Plays a short beep", beep},
  {"music", "Plays Tetris music", music},
  {"except", "Test exceptions", exception_test},
  {"golf", "Play Golf", gg_start_game},
  {"capy", "Print our cute mascot", print_mascot},
  {"test_alloc", "Test alloc syscall", test_alloc},
  {"test_check", "Test mem_check syscall", test_check},
};
size_t n_commands = sizeof(commands) / sizeof(command_t);

static int help() {
  printf(
    "Welcome to " COL_GREEN "carpinchOS" COL_RESET "!\n"
    "Available commands:\n\n"
  );

  for (int i = 0; i < n_commands; i++) {
    printf(
      COL_BLUE "%s" COL_RESET "\t- %s\n", commands[i].cmd, commands[i].desc
    );
  }

  printf(
    "\nPress the " COL_YELLOW "F1" COL_RESET " key any time to dump CPU state\n"
  );
  return 0;
}

static void write_prompt() { printf("> "); }

static void read_command(char *cmd) {
  int input_end = 0;
  uint32_t local_history_pointer = history_pointer;
  uint32_t write_pos = 0, back = 0;
  char temp[CMD_BUF_LEN];

  while (!input_end) {
    // Wait for input on stdin
    int len;
    do { len = read(temp, CMD_BUF_LEN); } while (!len);

    // Iterate the input and add to internal buffer, handling special characters
    char c;
    for (uint32_t read = 0; read < len; read++) {
      c = temp[read];

      if (c == '\n') {
        // Newline: end input
        input_end = 1;
        break;
      } else if (c == '\b') {
        // Backspace: delete last char from buffer
        if (write_pos > 0) write_pos--;
        cmd[write_pos] = 0;
      } else if (c == '\x1B') {
        // Escape char: handle escape sequences
        read += 2;     // All existing escape sequences are of the form "\x1B[X"
        c = temp[read];// Third character is the one we care about
        switch (c) {
          case 'A':
            // Up arrow
            if (local_history_pointer > 0) {
              char *last = command_history[--local_history_pointer];
              write_pos = strcpy(cmd, last);
            }
            break;
          case 'B':
            // Down arrow
            if (local_history_pointer < history_pointer - 1) {
              char *last = command_history[++local_history_pointer];
              write_pos = strcpy(cmd, last);
            }
            break;
          case 'C':
            // Right
            if (back > 0) {
              write_pos++;
              back--;
            }
            break;
          case 'D':
            // Left
            if (write_pos > 0) {
              write_pos--;
              back++;
            }
            break;
        }
      } else if (write_pos < CMD_BUF_LEN - 1) {
        // For any other char just add to internal buffer and advance write pointer
        // If we reached the end of the buffer, ignore any further input
        cmd[write_pos++] = c;
        if (back > 0) back--;

        // Make sure the command string is null-terminated
        cmd[write_pos + back] = 0;
      }
    }

    // Reset the cursor position and print the command so far to stdout
    io_blank_from(prompt_length);
    writes(cmd);
    io_setcursor(back ? IO_CURSOR_BLOCK : IO_CURSOR_UNDER);
    io_movecursor(-back);
  }

  // Insert a newline and reset the cursor
  static char newline = '\n';
  write(&newline, 1);
  io_setcursor(IO_CURSOR_UNDER);

  // Store the entered command in history, if it is different from the most recent one
  if (history_pointer == 0 ||
      strcmp(command_history[history_pointer - 1], cmd) != 0) {
    if (history_pointer == HISTORY_SIZE) {
      // If we reached the maximum history size, shift everything back
      // deleting the oldest command
      memcpy(
        command_history[0], command_history[1], (HISTORY_SIZE - 1) * CMD_BUF_LEN
      );
      history_pointer--;
    }

    strcpy(command_history[history_pointer++], cmd);
  }
}

static int run_command(const char *cmd) {
  char cmd_name[CMD_BUF_LEN];
  cmd = strsplit(cmd_name, cmd, ' ');

  if (cmd_name[0] == 0) return 0;

  // Linear search all commands. Not super efficient, but the number of
  // commands is quite small so we don't need to worry too much.
  int retcode = RET_UNKNOWN_CMD;
  for (int i = 0; i < n_commands; i++) {
    if (strcmp(cmd_name, commands[i].cmd) == 0) {
      retcode = commands[i].entryPoint(cmd);
      break;
    }
  }
  if (retcode == RET_UNKNOWN_CMD) {
    printf(
      COL_RED "Unknown command '%s'\n" COL_RESET "Hint: Type " COL_YELLOW
              "'help'" COL_RESET " for a list of available commands\n",
      cmd_name
    );
  } else if (retcode == RET_EXIT) {
    return 1;
  } else if (retcode != 0) {
    prompt_length = 2 + printf("[" COL_RED "%u" COL_RESET "] ", retcode);
  } else {
    prompt_length = 2;
  }


  return 0;
}

int start_shell() {
  char cmd_buf[CMD_BUF_LEN];

  // Run the shell
  int exit = 0;
  while (!exit) {
    write_prompt();
    read_command(cmd_buf);
    exit = run_command(cmd_buf);
  }

  // Return to kernel
  // This should almost never happen!
  printf("Goodbye!\n");

  return 0;
}
