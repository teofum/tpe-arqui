#include "kbd.h"
#include "vga.h"
#include <io.h>
#include <string.h>

#define LINE_LENGTH 128
#define MAX_LINES 64

char stdoutBuf[MAX_LINES][LINE_LENGTH + 1] = {0};
uint32_t line = 0;
uint32_t cursor = 0;

static void nextline() {
  cursor = 0;
  if (line < MAX_LINES - 1) {
    line++;
  } else {
    for (int i = 0; i < MAX_LINES - 1; i++) {
      memcpy(stdoutBuf[i], stdoutBuf[i + 1], LINE_LENGTH);
    }
  }
}

static void drawStdout() {
  vga_clear(0x000000);
  for (int i = 0; i <= line; i++)
    vga_text(
      0, i * vga_getfont()->lineHeight, stdoutBuf[i], 0xffffff, 0x000000,
      VGA_TEXT_BG
    );

  vga_present();
}

void io_putc(char c) {
  if (c == '\b') {
    if (cursor > 0) cursor--;
    stdoutBuf[line][cursor] = 0;
  } else if (c != '\n') {
    stdoutBuf[line][cursor++] = c;
    stdoutBuf[line][cursor] = 0;
  }
  if (cursor == LINE_LENGTH || c == '\n') nextline();

  drawStdout();
}

uint32_t io_writes(const char *str) {
  char c;
  while ((c = *str++)) {
    if (c == '\b') {
      if (cursor > 0) cursor--;
      stdoutBuf[line][cursor] = 0;
    } else if (c != '\n') {
      stdoutBuf[line][cursor++] = c;
      stdoutBuf[line][cursor] = 0;
    }
    if (cursor == LINE_LENGTH || c == '\n') nextline();
  }

  drawStdout();
  return 0;
}

uint32_t io_write(const char *str, uint32_t len) {
  char c;
  for (int i = 0; i < len; i++) {
    c = *str++;
    if (c == '\b') {
      if (cursor > 0) cursor--;
      stdoutBuf[line][cursor] = 0;
    } else if (c != '\n') {
      stdoutBuf[line][cursor++] = c;
      stdoutBuf[line][cursor] = 0;
    }
    if (cursor == LINE_LENGTH || c == '\n') nextline();
  }

  drawStdout();
  return 0;
}

void io_clear() {
  for (int i = 0; i <= line; i++) {
    for (int j = 0; j < LINE_LENGTH; j++) stdoutBuf[i][j] = 0;
  }
  line = cursor = 0;
}

uint32_t io_read(char *buf, uint32_t len) {
  uint32_t readChars = 0;
  int c;
  while ((c = kbd_getchar()) != -1 && readChars < len) {
    if (c != 0) buf[readChars++] = c;
  }

  return readChars;
}
