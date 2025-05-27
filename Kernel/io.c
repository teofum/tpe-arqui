#include "kbd.h"
#include "vga.h"
#include <io.h>
#include <string.h>

#define TEXT_BG 0x000000
#define TEXT_FG 0xffffff

uint8_t textFramebuffer[VGA_WIDTH * VGA_HEIGHT * 3] = {0};
const vga_font_t *textFont;

uint32_t cur_y = 0;
uint32_t cur_x = 0;

static void nextline() {
  cur_x = 0;
  cur_y += textFont->lineHeight;

  int32_t remaining = VGA_HEIGHT - (int32_t) (cur_y + textFont->lineHeight);
  if (remaining <= 0) {
    uint16_t offsetLines = -remaining;

    uint32_t offset = offsetLines * VGA_WIDTH * 3;
    memcpy(
      textFramebuffer, textFramebuffer + offset,
      VGA_WIDTH * VGA_HEIGHT * 3 - offset
    );

    vga_rect(
      0, VGA_HEIGHT - offsetLines, VGA_WIDTH - 1, VGA_HEIGHT - 1, TEXT_BG, 0
    );

    cur_y -= offsetLines;
  }
}

static inline void putcImpl(char c) {
  if (c == '\b') {
    if (cur_x > 0) cur_x -= textFont->charWidth;
    vga_rect(
      cur_x, cur_y, cur_x + textFont->charWidth - 1,
      cur_y + textFont->charHeight - 1, TEXT_BG, 0
    );
  } else if (c != '\n') {
    vga_char(cur_x, cur_y, c, TEXT_FG, TEXT_BG, VGA_TEXT_BG);
    cur_x += textFont->charWidth;
  }
  if (cur_x >= VGA_WIDTH || c == '\n') nextline();
}

void io_init() { textFont = vga_fontDefault; }

void io_putc(char c) {
  vga_setFramebuffer(textFramebuffer);

  putcImpl(c);

  vga_present();
  vga_setFramebuffer(NULL);
}

uint32_t io_writes(const char *str) {
  vga_setFramebuffer(textFramebuffer);

  char c;
  while ((c = *str++)) { putcImpl(c); }

  vga_present();
  vga_setFramebuffer(NULL);
  return 0;
}

uint32_t io_write(const char *str, uint32_t len) {
  vga_setFramebuffer(textFramebuffer);

  char c;
  for (int i = 0; i < len; i++) {
    c = *str++;
    putcImpl(c);
  }

  vga_present();
  vga_setFramebuffer(NULL);
  return 0;
}

void io_clear() {
  vga_setFramebuffer(textFramebuffer);
  vga_clear(0x000000);
  vga_present();
  vga_setFramebuffer(NULL);

  cur_x = cur_y = 0;
}

uint32_t io_read(char *buf, uint32_t len) {
  uint32_t readChars = 0;
  int c;
  while ((c = kbd_getchar()) != -1 && readChars < len) {
    if (c != 0) buf[readChars++] = c;
  }

  return readChars;
}
