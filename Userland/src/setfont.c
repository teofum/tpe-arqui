#include <io.h>
#include <setfont.h>
#include <strings.h>
#include <vga.h>

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

int setfont(uint64_t argc, char *const *argv) {
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

  // TODO either allocate this dynamically, or find a way to avoid this alloc entirely
  char name[100];
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
