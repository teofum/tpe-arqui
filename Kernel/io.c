#include "kbd.h"
#include "vga.h"
#include <io.h>
#include <string.h>

#define TAB_SIZE 8

#define DEFAULT_BG 0x000000
#define DEFAULT_FG 0xd8d8d8

/*
 * The I/O subsystem keeps its own framebuffer, so it can preserve text written
 * to the screen after graphics mode functions are used (for example, writing
 * to the main framebuffer directly).
 * This takes up a bunch of memory (2.25 MB), but it allows us to both draw
 * text very efficiently and preserve it when an application uses graphics mode.
 */
uint8_t textFramebuffer[VGA_WIDTH * VGA_HEIGHT * 3] = {0};
const vga_font_t *textFont;

/*
 * Text drawing cursor
 */
uint32_t cur_y = 0;
uint32_t cur_x = 0;

/*
 * Foreground and background colors
 */
uint32_t background = DEFAULT_BG;
uint32_t foreground = DEFAULT_FG;

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
      0, VGA_HEIGHT - offsetLines, VGA_WIDTH - 1, VGA_HEIGHT - 1, DEFAULT_BG, 0
    );

    cur_y -= offsetLines;
  }
}

static inline void putcImpl(char c) {
  if (c == '\b') {
    if (cur_x > 0) cur_x -= textFont->charWidth;
    vga_rect(
      cur_x, cur_y, cur_x + textFont->charWidth - 1,
      cur_y + textFont->charHeight - 1, DEFAULT_BG, 0
    );
  } else if (c == '\t') {
    cur_x += (TAB_SIZE * textFont->charWidth) -
             (cur_x % (TAB_SIZE * textFont->charWidth));
  } else if (c != '\n') {
    vga_char(cur_x, cur_y, c, foreground, background, VGA_TEXT_BG);
    cur_x += textFont->charWidth;
  }
  if (cur_x >= VGA_WIDTH || c == '\n') nextline();
}

void io_init() { textFont = vga_fontDefault; }

void io_putc(char c) {
  vga_setFramebuffer(textFramebuffer);
  const vga_font_t *lastFont = vga_font(textFont);

  putcImpl(c);

  vga_font(lastFont);
  vga_copy(NULL, textFramebuffer);
  vga_setFramebuffer(NULL);
  vga_present();
}

static const char *parseColorEscape(const char *str) {
  char c;

  // Color escape sequence
  uint8_t mode = 0;
  color_t color = 0x000000;
  color_t channel = 0;
  uint8_t shift = 16;
  while ((c = *str++) && c != ';') {
    if (c == 'b' || c == 'B') {
      mode = 1;
    } else if (c == 'f' || c == 'F') {
      mode = 0;
    } else if (c == 'r' || c == 'R') {
      color = mode == 0 ? DEFAULT_FG : DEFAULT_BG;
    } else if (c == ',') {
      color |= channel << shift;
      shift -= 8;
      channel = 0;
    } else if (c >= '0' && c <= '9') {
      if (channel > 0 || c != '0') {
        channel *= 10;
        channel += c - '0';
      }
    }
  }
  color |= channel;

  if (mode == 0) {
    foreground = color;
  } else {
    background = color;
  }

  return str;
}

uint32_t io_writes(const char *str) {
  vga_setFramebuffer(textFramebuffer);
  const vga_font_t *lastFont = vga_font(textFont);

  char c;
  while ((c = *str++)) {
    if (c == 0x1A) {
      str = parseColorEscape(str);
    } else {
      putcImpl(c);
    }
  }

  background = DEFAULT_BG;
  foreground = DEFAULT_FG;
  vga_font(lastFont);
  vga_copy(NULL, textFramebuffer);
  vga_setFramebuffer(NULL);
  vga_present();
  return 0;
}

uint32_t io_write(const char *str, uint32_t len) {
  vga_setFramebuffer(textFramebuffer);
  const vga_font_t *lastFont = vga_font(textFont);

  char c;
  const char *end = str + len;
  while (str < end && (c = *str++)) {
    if (c == 0x1A) {
      str = parseColorEscape(str);
    } else {
      putcImpl(c);
    }
  }

  background = DEFAULT_BG;
  foreground = DEFAULT_FG;
  vga_font(lastFont);
  vga_copy(NULL, textFramebuffer);
  vga_setFramebuffer(NULL);
  vga_present();
  return 0;
}

void io_clear() {
  vga_setFramebuffer(textFramebuffer);
  vga_clear(0x000000);
  vga_copy(NULL, textFramebuffer);
  vga_setFramebuffer(NULL);
  vga_present();

  cur_x = cur_y = 0;
}

uint32_t io_read(char *buf, uint32_t len) {
  uint32_t readChars = 0;
  int c;
  while ((c = kbd_getchar()) != -1 && readChars < len) {
    if (c != 0) {
      if (isSpecialCharcode(c)) {
        // Make sure there's enough room in the buffer to actually fit the
        // escape sequence, if there isn't we just drop it
        // This can probably be handled better, oh well
        if (readChars >= len - 2) return readChars;

        // Handle special keys by writing escape sequences to stdin
        // Code that uses read() should handle these escape sequences
        uint8_t key = getKey(c);
        switch (key) {
          case KEY_ARROW_UP:
            buf[readChars++] = 0x1B;// ESC
            buf[readChars++] = '[';
            buf[readChars++] = 'A';
            break;
          case KEY_ARROW_DOWN:
            buf[readChars++] = 0x1B;// ESC
            buf[readChars++] = '[';
            buf[readChars++] = 'B';
            break;
          case KEY_ARROW_LEFT:
            buf[readChars++] = 0x1B;// ESC
            buf[readChars++] = '[';
            buf[readChars++] = 'D';
            break;
          case KEY_ARROW_RIGHT:
            buf[readChars++] = 0x1B;// ESC
            buf[readChars++] = '[';
            buf[readChars++] = 'C';
            break;
        }
      } else {
        buf[readChars++] = c;
      }
    }
  }

  return readChars;
}

void io_setfont(io_font_t font) {
  // We use an enum here so we can call this from a syscall, since we don't
  // have the pointers to VGA font data in userland
  switch (font) {
    case IO_FONT_DEFAULT:
      textFont = vga_fontDefault;
      break;
    case IO_FONT_TINY:
      textFont = vga_fontTiny;
      break;
    case IO_FONT_TINY_BOLD:
      textFont = vga_fontTinyBold;
      break;
    case IO_FONT_SMALL:
      textFont = vga_fontSmall;
      break;
    case IO_FONT_LARGE:
      textFont = vga_fontLarge;
      break;
    case IO_FONT_ALT:
      textFont = vga_fontAlt;
      break;
    case IO_FONT_ALT_BOLD:
      textFont = vga_fontAltBold;
      break;
    case IO_FONT_FUTURE:
      textFont = vga_fontFuture;
      break;
    case IO_FONT_OLD:
      textFont = vga_fontOld;
      break;
  }

  vga_setFramebuffer(textFramebuffer);
  int32_t remaining = VGA_HEIGHT - (int32_t) (cur_y + textFont->lineHeight);
  if (remaining <= 0) {
    uint16_t offsetLines = -remaining;

    uint32_t offset = offsetLines * VGA_WIDTH * 3;
    memcpy(
      textFramebuffer, textFramebuffer + offset,
      VGA_WIDTH * VGA_HEIGHT * 3 - offset
    );

    vga_rect(
      0, VGA_HEIGHT - offsetLines, VGA_WIDTH - 1, VGA_HEIGHT - 1, DEFAULT_BG, 0
    );

    cur_y -= offsetLines;
  }
  vga_copy(NULL, textFramebuffer);
  vga_setFramebuffer(NULL);
  vga_present();
}
