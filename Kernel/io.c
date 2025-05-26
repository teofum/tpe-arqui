#include "vga.h"
#include <io.h>
#include <string.h>

#define LINE_LENGTH 128
#define MAX_LINES 64

char stdoutBuf[MAX_LINES][LINE_LENGTH + 1] = {0};
uint32_t line = 0;
uint32_t cursor = 0;

void nextline() {
  cursor = 0;
  if (line < MAX_LINES - 1) {
    line++;
  } else {
    for (int i = 0; i < MAX_LINES - 1; i++) {
      memcpy(stdoutBuf[i], stdoutBuf[i + 1], LINE_LENGTH);
    }
  }
}

void drawStdout() {
  for (int i = 0; i < MAX_LINES; i++)
    vga_text(0, i * 16, stdoutBuf[i], 0xffffff, 0x000000, VGA_TEXT_BG);
}

void io_putc(char c) {
  if (c != '\n') stdoutBuf[line][cursor++] = c;
  if (cursor == LINE_LENGTH || c == '\n') nextline();

  drawStdout();
}

uint32_t io_writes(const char *str) {
  char c;
  while ((c = *str++)) {
    if (c != '\n') stdoutBuf[line][cursor++] = c;
    if (cursor == LINE_LENGTH || c == '\n') nextline();
  }

  drawStdout();
  return 0;
}

uint32_t io_write(const char *str, uint32_t len) {
  char c;
  for (int i = 0; i < len; i++) {
    c = *str++;
    if (c != '\n') stdoutBuf[line][cursor++] = c;
    if (cursor == LINE_LENGTH || c == '\n') nextline();
  }

  drawStdout();
  return 0;
}
