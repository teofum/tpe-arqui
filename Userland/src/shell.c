#include <gfxdemo.h>
#include <golf_game.h>
#include <io.h>
#include <kbd.h>
#include <mem.h>
#include <print.h>
#include <proc.h>
#include <process.h>
#include <shell.h>
#include <sound.h>
#include <status.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>

#define SHELL_VERSION "1.1.0"

#define CMD_BUF_LEN 64
#define HISTORY_SIZE 64

extern const char *mascot;

typedef struct {
  const char *cmd;
  const char *desc;
  proc_entrypoint_t entry_point;
} program_t;

typedef struct {
  uint64_t argc;
  char *const *argv;

  int background : 1;
} args_t;

static char command_history[HISTORY_SIZE][CMD_BUF_LEN];
static uint32_t history_pointer = 0;
static uint32_t prompt_length = 2;

static int echo(uint64_t argc, char *const *argv) {
  if (argc > 1) {
    for (size_t i = 1; i < argc; i++) { printf("%s ", argv[i]); }
    write("\n", 1);
  }

  return 0;
}

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

static int setfont(uint64_t argc, char *const *argv) {
  if (argc < 2) {
    printf(COL_RED "Missing font name\n" COL_RESET
                   "Usage: setfont <font name>\n"
                   "Hint: Type " COL_YELLOW "'setfont ls'" COL_RESET
                   " for a list of fonts\n");
    return 1;
  }

  if (strcmp(argv[1], "ls") == 0) {
    for (int i = 0; i < n_fonts; i++) {
      printf(COL_BLUE "%s\n", fonts[i].name);
    }
    return 0;
  }

  char name[CMD_BUF_LEN];
  char *w = name;
  for (size_t i = 1; i < argc; i++) {
    w += sprintf(w, "%s", argv[i]);
    if (i < argc - 1) w += sprintf(w, " ");
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

static int status(uint64_t argc, char *const *argv) {
  if (argc < 2) {
    printf("Usage: status <on/off>\n");
    return 1;
  }

  if (!strcmp(argv[1], "off")) {
    status_set_enabled(0);
    io_clear();
    return 0;
  } else if (!strcmp(argv[1], "on")) {
    status_set_enabled(1);
    io_clear();
    return 0;
  }

  printf(
    COL_RED "Invalid argument '%s'\n" COL_RESET "Usage: status <on|off>\n",
    argv[1]
  );

  return 2;
}

extern void _throw_00();
extern void _throw_06();
extern void _regdump_test();
int exception_test(uint64_t argc, char *const *argv) {
  if (argc < 2) {
    printf("Usage: except <0|6|test_regdump>\n");
    return 1;
  }

  if (!strcmp(argv[1], "0")) {
    _throw_00();
  } else if (!strcmp(argv[1], "6")) {
    _throw_06();
  } else if (!strcmp(argv[1], "test_regdump")) {
    _regdump_test();
  } else {
    printf(
      COL_RED "Invalid exception type '%s'\n" COL_RESET
              "Usage: except <0|6|test_regdump>\n",
      argv[1]
    );
    return 1;
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

static int mem() {
  size_t total, used, free;
  mem_status(&total, &used, &free);

  size_t used_pct = (used * 100) / total;
  size_t free_pct = (free * 100) / total;

  printf(
    COL_GRAY "Total: " COL_RESET "%lu bytes\n" COL_GREEN "Used:  " COL_RESET
             "%lu bytes (%lu%%)\n" COL_BLUE "Free:  " COL_RESET
             "%lu bytes (%lu%%)\n",
    total, used, used_pct, free, free_pct
  );

  return 0;
}

static int print_test() {
  printf("my pid is %u\n", getpid());
  for (uint32_t i = 0; i < 5; i++) {
    printf("Press a key... ");
    kbd_get_key_event();
    printf("%u\n", i);
  }
  write("\n", 1);

  return 0;
}

static int timer_test(uint64_t argc, char *const *argv) {
  printf("my pid is %u\n", getpid());
  uint32_t i = 0;
  while (1) {
    // shitty delay TODO have a real timer
    for (uint32_t j = 0; j < 5000; j++) yield();
    printf("%u %s\n", i++, argv[1]);
  }
  write("\n", 1);

  return 0;
}

static int help();
static program_t commands[] = {
  {"help", "Display this help message", help},
  {"echo", "Print arguments to stdout", echo},
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
  {"mem", "Display memory status", mem},
  {"test1", "for bg testing, with kb input", print_test},
  {"test2", "for bg testing, with timer", timer_test},
  {"proc", "Manage processes", proc},
};
static size_t n_commands = sizeof(commands) / sizeof(program_t);

static int help() {
  printf("Available commands:\n\n");

  for (int i = 0; i < n_commands; i++) {
    printf(
      COL_BLUE "%s" COL_RESET "\t- %s\n", commands[i].cmd, commands[i].desc
    );
  }

  printf("\nPress the " COL_YELLOW "F1" COL_RESET
         " key any time to dump CPU state\n");
  return 0;
}

static void write_prompt() { printf("$ "); }

static void read_command(char *cmd) {
  int input_end = 0;
  uint32_t local_history_pointer = history_pointer;
  uint32_t write_pos = 0, back = 0;
  char temp[4];// Buffer big enough for multi char escape sequences

  while (!input_end) {
    // Wait for input on stdin
    read(temp, 1);

    // Iterate the input and add to internal buffer, handling special characters
    char c = temp[0];
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
      // All existing escape sequences are of the form "\x1B[X"
      // Third character is the one we care about
      c = temp[2];
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

static program_t *find_program(const char *cmd_name) {
  for (int i = 0; i < n_commands; i++)
    if (strcmp(cmd_name, commands[i].cmd) == 0) return &commands[i];

  return NULL;
}

static args_t make_args(const char *cmd) {
  uint64_t argc = 1;

  for (size_t i = 0; cmd[i] != 0; i++) {
    if (cmd[i] == ' ' && cmd[i + 1] != '&') argc++;
  }

  char **argv = mem_alloc(argc * sizeof(char *));
  char *arg_str = mem_alloc(CMD_BUF_LEN);

  int background = 0;
  int i = 0, j = 0;
  argv[0] = arg_str;
  for (; cmd[i] != 0; i++) {
    if (cmd[i] == ' ') {
      arg_str[i] = 0;
      if (cmd[i + 1] == '&') {
        background = 1;
      } else {
        argv[++j] = &arg_str[i + 1];
      }
    } else {
      arg_str[i] = cmd[i];
    }
  }
  arg_str[i] = 0;

  return (args_t) {
    .argc = argc,
    .argv = argv,
    .background = background,
  };
}

static void free_args(args_t *args) {
  mem_free(args->argv[0]);
  mem_free((void *) args->argv);
}

static int run_command(const char *cmd) {
  char program_name[CMD_BUF_LEN];
  strsplit(program_name, cmd, ' ');

  if (program_name[0] == 0) return 0;

  program_t *program = find_program(program_name);
  if (!program) {
    printf(
      COL_RED "Unknown command '%s'\n" COL_RESET "Hint: Type " COL_YELLOW
              "'help'" COL_RESET " for a list of available commands\n",
      program_name
    );

    return 0;
  }

  char *const *argv;
  args_t args = make_args(cmd);

  pid_t pid =
    proc_spawn(program->entry_point, args.argc, args.argv, DEFAULT_PRIORITY);
  int background = args.background;
  free_args(&args);

  if (!background) {
    int return_value = proc_wait(pid);
    prompt_length = 2;
    if (return_value != 0) {
      prompt_length += printf("[" COL_RED "%u" COL_RESET "] ", return_value);
    }
  }

  return 0;
}

int cash() {
  char cmd_buf[CMD_BUF_LEN];

  printf("Welcome to " COL_GREEN "carpinchOS\n");
  printf("cash v" SHELL_VERSION " | " COL_GREEN "Capybara Shell\n");

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
