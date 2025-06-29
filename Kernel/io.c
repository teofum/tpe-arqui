#include <io.h>
#include <kbd.h>
#include <status.h>
#include <string.h>
#include <vga.h>

#define TAB_SIZE 8
#define CURSOR_HEIGHT 2

#define DEFAULT_BG 0x000000
#define DEFAULT_FG 0xd8d8d8

#define copyToMainFramebuffer()                                                \
  vga_copy(NULL, io_framebuffer, status_enabled() ? STATUS_HEIGHT : 0)

/*
 * The I/O subsystem keeps its own framebuffer, so it can preserve text written
 * to the screen after graphics mode functions are used (for example, writing
 * to the main framebuffer directly).
 * This takes up a bunch of memory (3 MB), but it allows us to both draw
 * text very efficiently and preserve it when an application uses graphics mode.
 */
static uint8_t io_framebuffer[FRAMEBUFFER_SIZE] = {0};
static vga_font_t io_textFont;

/*
 * Text drawing cursor
 */
static uint32_t cur_y = 0;
static uint32_t cur_x = 0;

/*
 * Foreground and background colors
 */
static uint32_t io_background = DEFAULT_BG;
static uint32_t io_foreground = DEFAULT_FG;

static uint8_t io_cursorStyle = IO_CURSOR_UNDER;

static void nextline() {
  vga_fontPtr_t font = vga_getfont(io_textFont);

  cur_x = 0;
  cur_y += font->lineHeight;

  uint32_t maxHeight = VGA_HEIGHT - (status_enabled() ? STATUS_HEIGHT : 0);
  int32_t remaining = maxHeight - (int32_t) (cur_y + font->lineHeight);
  if (remaining <= 0) {
    uint16_t offsetLines = -remaining;

    uint32_t offset = offsetLines * OFFSET_Y;
    memcpy(
      io_framebuffer, io_framebuffer + offset, OFFSET_Y * VGA_HEIGHT - offset
    );

    vga_rect(
      0, maxHeight - offsetLines, VGA_WIDTH - 1, maxHeight - 1, DEFAULT_BG, 0
    );

    cur_y -= offsetLines;
  }
}

/*
 * Cursor should be drawn to main framebuffer *after* copying stdout framebuffer.
 * This way it doesn't persist when we draw more text.
 */
static inline void drawCursor() {
  vga_fontPtr_t font = vga_getfont(io_textFont);
  uint32_t cursor_y = cur_y + (status_enabled() ? STATUS_HEIGHT : 0);

  switch (io_cursorStyle) {
    case IO_CURSOR_UNDER:
      vga_rect(
        cur_x, cursor_y + font->charHeight - CURSOR_HEIGHT,
        cur_x + font->charWidth, cursor_y + font->charHeight - 1, io_foreground,
        0
      );
      break;
    case IO_CURSOR_FRAME:
      vga_frame(
        cur_x, cursor_y, cur_x + font->charWidth, cursor_y + font->charHeight,
        io_foreground, 0
      );
      break;
    case IO_CURSOR_BLOCK:
      vga_rect(
        cur_x, cursor_y, cur_x + font->charWidth, cursor_y + font->charHeight,
        io_foreground | 0x80000000, VGA_ALPHA_BLEND
      );
      break;
  }
}

static inline void putcImpl(char c) {
  vga_fontPtr_t font = vga_getfont(io_textFont);

  if (c == '\b') {
    if (cur_x > 0) cur_x -= font->charWidth;
    vga_rect(
      cur_x, cur_y, cur_x + font->charWidth - 1, cur_y + font->charHeight - 1,
      DEFAULT_BG, 0
    );
  } else if (c == '\t') {
    cur_x +=
      (TAB_SIZE * font->charWidth) - (cur_x % (TAB_SIZE * font->charWidth));
  } else if (c != '\n') {
    vga_char(cur_x, cur_y, c, io_foreground, io_background, VGA_TEXT_BG);
    cur_x += font->charWidth;
  }
  if (cur_x >= VGA_WIDTH || c == '\n') nextline();
}

void io_blankFrom(uint32_t x) {
  vga_fontPtr_t font = vga_getfont(io_textFont);

  vga_framebuffer_t currentFB = vga_setFramebuffer(io_framebuffer);

  cur_x = x * font->charWidth;
  if (cur_x >= VGA_WIDTH) cur_x = VGA_WIDTH - font->charWidth;

  vga_rect(
    cur_x, cur_y, VGA_WIDTH - 1, cur_y + font->lineHeight - 1, DEFAULT_BG, 0
  );

  vga_setFramebuffer(currentFB);
}

void io_putc(char c) {
  vga_framebuffer_t currentFB = vga_setFramebuffer(io_framebuffer);
  vga_font_t lastFont = vga_font(io_textFont);

  putcImpl(c);

  vga_font(lastFont);
  copyToMainFramebuffer();
  vga_setFramebuffer(NULL);
  drawCursor();
  vga_present();
  vga_setFramebuffer(currentFB);
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
    io_foreground = color;
  } else {
    io_background = color;
  }

  return str;
}

uint32_t io_writes(const char *str) {
  vga_framebuffer_t currentFB = vga_setFramebuffer(io_framebuffer);
  vga_font_t lastFont = vga_font(io_textFont);

  char c;
  uint32_t written = 0;
  while ((c = *str++)) {
    if (c == 0x1A) {
      str = parseColorEscape(str);
    } else {
      putcImpl(c);
      written++;
    }
  }

  io_background = DEFAULT_BG;
  io_foreground = DEFAULT_FG;
  vga_font(lastFont);
  copyToMainFramebuffer();
  vga_setFramebuffer(NULL);
  drawCursor();
  vga_present();
  vga_setFramebuffer(currentFB);

  return written;
}

uint32_t io_write(const char *str, uint32_t len) {
  vga_framebuffer_t currentFB = vga_setFramebuffer(io_framebuffer);
  vga_font_t lastFont = vga_font(io_textFont);

  char c;
  const char *end = str + len;
  uint32_t written = 0;
  while (str < end && (c = *str++)) {
    if (c == 0x1A) {
      str = parseColorEscape(str);
    } else {
      putcImpl(c);
      written++;
    }
  }

  io_background = DEFAULT_BG;
  io_foreground = DEFAULT_FG;
  vga_font(lastFont);
  copyToMainFramebuffer();
  vga_setFramebuffer(NULL);
  drawCursor();
  vga_present();
  vga_setFramebuffer(currentFB);

  return written;
}

void io_clear() {
  vga_framebuffer_t currentFB = vga_setFramebuffer(io_framebuffer);
  vga_clear(0x000000);
  copyToMainFramebuffer();
  vga_setFramebuffer(NULL);
  drawCursor();
  vga_present();
  vga_setFramebuffer(currentFB);

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

void io_setfont(vga_font_t font) {
  io_textFont = font;
  vga_fontPtr_t fontData = vga_getfont(io_textFont);

  vga_framebuffer_t currentFB = vga_setFramebuffer(io_framebuffer);
  uint32_t maxHeight = VGA_HEIGHT - (status_enabled() ? STATUS_HEIGHT : 0);
  int32_t remaining = maxHeight - (int32_t) (cur_y + fontData->lineHeight);
  if (remaining <= 0) {
    uint16_t offsetLines = -remaining;

    uint32_t offset = offsetLines * OFFSET_Y;
    memcpy(
      io_framebuffer, io_framebuffer + offset, OFFSET_Y * VGA_HEIGHT - offset
    );

    vga_rect(
      0, maxHeight - offsetLines, VGA_WIDTH - 1, maxHeight - 1, DEFAULT_BG, 0
    );

    cur_y -= offsetLines;
  }

  copyToMainFramebuffer();
  vga_setFramebuffer(NULL);
  drawCursor();
  vga_present();
  vga_setFramebuffer(currentFB);
}

void io_setcursor(io_cursor_t cursor) { io_cursorStyle = cursor; }

void io_movecursor(int32_t dx) {
  vga_fontPtr_t font = vga_getfont(io_textFont);

  dx *= font->charWidth;
  if (dx < 0 && cur_x < -dx) {
    cur_x = 0;
  } else {
    cur_x += dx;
    if (cur_x >= VGA_WIDTH) nextline();
  }

  copyToMainFramebuffer();
  vga_framebuffer_t currentFB = vga_setFramebuffer(NULL);
  drawCursor();
  vga_present();
  vga_setFramebuffer(currentFB);
}
