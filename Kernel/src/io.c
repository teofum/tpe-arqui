#include <io.h>
#include <kbd.h>
#include <status.h>
#include <stdint.h>
#include <string.h>
#include <vga.h>

#define TAB_SIZE 8
#define CURSOR_HEIGHT 2

#define DEFAULT_BG 0x000000
#define DEFAULT_FG 0xd8d8d8

#define copy_to_main_fb()                                                      \
  vga_copy(NULL, io_framebuffer, status_enabled() ? STATUS_HEIGHT : 0)

/*
 * The I/O subsystem keeps its own framebuffer, so it can preserve text written
 * to the screen after graphics mode functions are used (for example, writing
 * to the main framebuffer directly).
 * This takes up a bunch of memory (3 MB), but it allows us to both draw
 * text very efficiently and preserve it when an application uses graphics mode.
 */
static vga_framebuffer_t io_framebuffer;
static vga_font_t io_text_font;

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

static uint8_t io_cursor_style = IO_CURSOR_UNDER;

// TODO REMOVE THIS FROM HERE!!!!!!
// IO module should not be doing direct fb manipulation!!!!!!!
struct vga_framebuffer_cdt_t {
  uint32_t width;
  uint32_t height;
  uint8_t data[];
};

static void nextline() {
  vga_font_ptr_t font = vga_getfont(io_text_font);

  cur_x = 0;
  cur_y += font->line_height;

  uint32_t max_height = VGA_HEIGHT - (status_enabled() ? STATUS_HEIGHT : 0);
  int32_t remaining = max_height - (int32_t) (cur_y + font->line_height);
  if (remaining <= 0) {
    uint16_t offset_lines = -remaining;

    uint32_t offset = offset_lines * OFFSET_Y;
    memcpy(
      io_framebuffer->data, io_framebuffer->data + offset,
      OFFSET_Y * VGA_HEIGHT - offset
    );

    vga_rect(
      0, max_height - offset_lines, VGA_WIDTH - 1, max_height - 1, DEFAULT_BG, 0
    );

    cur_y -= offset_lines;
  }
}

/*
 * Cursor should be drawn to main framebuffer *after* copying stdout framebuffer.
 * This way it doesn't persist when we draw more text.
 */
static inline void draw_cursor() {
  vga_font_ptr_t font = vga_getfont(io_text_font);
  uint32_t cursor_y = cur_y + (status_enabled() ? STATUS_HEIGHT : 0);

  switch (io_cursor_style) {
    case IO_CURSOR_UNDER:
      vga_rect(
        cur_x, cursor_y + font->char_height - CURSOR_HEIGHT,
        cur_x + font->char_width, cursor_y + font->char_height - 1,
        io_foreground, 0
      );
      break;
    case IO_CURSOR_FRAME:
      vga_frame(
        cur_x, cursor_y, cur_x + font->char_width, cursor_y + font->char_height,
        io_foreground, 0
      );
      break;
    case IO_CURSOR_BLOCK:
      vga_rect(
        cur_x, cursor_y, cur_x + font->char_width, cursor_y + font->char_height,
        io_foreground | 0x80000000, VGA_ALPHA_BLEND
      );
      break;
  }
}

static inline void putc_impl(char c) {
  vga_font_ptr_t font = vga_getfont(io_text_font);

  if (c == '\b') {
    if (cur_x > 0) cur_x -= font->char_width;
    vga_rect(
      cur_x, cur_y, cur_x + font->char_width - 1, cur_y + font->char_height - 1,
      DEFAULT_BG, 0
    );
  } else if (c == '\t') {
    cur_x +=
      (TAB_SIZE * font->char_width) - (cur_x % (TAB_SIZE * font->char_width));
  } else if (c != '\n') {
    vga_char(cur_x, cur_y, c, io_foreground, io_background, VGA_TEXT_BG);
    cur_x += font->char_width;
  }
  if (cur_x >= VGA_WIDTH || c == '\n') nextline();
}

void io_blank_from(uint32_t x) {
  vga_font_ptr_t font = vga_getfont(io_text_font);

  vga_framebuffer_t current_fb = vga_set_framebuffer(io_framebuffer);

  cur_x = x * font->char_width;
  if (cur_x >= VGA_WIDTH) cur_x = VGA_WIDTH - font->char_width;

  vga_rect(
    cur_x, cur_y, VGA_WIDTH - 1, cur_y + font->line_height - 1, DEFAULT_BG, 0
  );

  vga_set_framebuffer(current_fb);
}

static const char *parse_color_escape(const char *str) {
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

void io_init() { io_framebuffer = vga_create_framebuffer(VGA_AUTO, VGA_AUTO); }

static uint32_t io_writes_tty(const char *str) {
  vga_framebuffer_t current_fb = vga_set_framebuffer(io_framebuffer);
  vga_font_t last_font = vga_font(io_text_font);

  char c;
  uint32_t written = 0;
  while ((c = *str++)) {
    if (c == 0x1A) {
      str = parse_color_escape(str);
    } else {
      putc_impl(c);
      written++;
    }
  }

  io_background = DEFAULT_BG;
  io_foreground = DEFAULT_FG;
  vga_font(last_font);
  copy_to_main_fb();
  vga_set_framebuffer(NULL);
  draw_cursor();
  vga_present();
  vga_set_framebuffer(current_fb);

  return written;
}

uint32_t io_writes(const char *str) { return io_writes_tty(str); }

static uint32_t io_write_tty(const char *str, uint32_t len) {
  vga_framebuffer_t current_fb = vga_set_framebuffer(io_framebuffer);
  vga_font_t last_font = vga_font(io_text_font);

  char c;
  const char *end = str + len;
  uint32_t written = 0;
  while (str < end && (c = *str++)) {
    if (c == 0x1A) {
      str = parse_color_escape(str);
    } else {
      putc_impl(c);
      written++;
    }
  }

  io_background = DEFAULT_BG;
  io_foreground = DEFAULT_FG;
  vga_font(last_font);
  copy_to_main_fb();
  vga_set_framebuffer(NULL);
  draw_cursor();
  vga_present();
  vga_set_framebuffer(current_fb);

  return written;
}

uint32_t io_write(const char *str, uint32_t len) {
  return io_write_tty(str, len);
}

void io_clear() {
  vga_framebuffer_t current_fb = vga_set_framebuffer(io_framebuffer);
  vga_clear(0x000000);
  copy_to_main_fb();
  vga_set_framebuffer(NULL);
  draw_cursor();
  vga_present();
  vga_set_framebuffer(current_fb);

  cur_x = cur_y = 0;
}

static uint32_t io_read_tty(char *buf, uint32_t len) {
  uint32_t read_chars = 0;
  int c;
  while (read_chars < len && (c = kbd_getchar()) != KBD_EOF) {
    if (c != 0) {
      if (is_special_charcode(c)) {
        // Make sure there's enough room in the buffer to actually fit the
        // escape sequence, if there isn't we just drop it
        // This can probably be handled better, oh well
        if (read_chars >= len - 2) return read_chars;

        // Handle special keys by writing escape sequences to stdin
        // Code that uses read() should handle these escape sequences
        uint8_t key = getKey(c);
        switch (key) {
          case KEY_ARROW_UP:
            buf[read_chars++] = 0x1B;// ESC
            buf[read_chars++] = '[';
            buf[read_chars++] = 'A';
            break;
          case KEY_ARROW_DOWN:
            buf[read_chars++] = 0x1B;// ESC
            buf[read_chars++] = '[';
            buf[read_chars++] = 'B';
            break;
          case KEY_ARROW_LEFT:
            buf[read_chars++] = 0x1B;// ESC
            buf[read_chars++] = '[';
            buf[read_chars++] = 'D';
            break;
          case KEY_ARROW_RIGHT:
            buf[read_chars++] = 0x1B;// ESC
            buf[read_chars++] = '[';
            buf[read_chars++] = 'C';
            break;
        }
      } else {
        buf[read_chars++] = c;
      }
    }
  }

  return read_chars;
}

uint32_t io_read(char *buf, uint32_t len) { return io_read_tty(buf, len); }

void io_setfont(vga_font_t font) {
  io_text_font = font;
  vga_font_ptr_t fontData = vga_getfont(io_text_font);

  vga_framebuffer_t current_fb = vga_set_framebuffer(io_framebuffer);
  uint32_t max_height = VGA_HEIGHT - (status_enabled() ? STATUS_HEIGHT : 0);
  int32_t remaining = max_height - (int32_t) (cur_y + fontData->line_height);
  if (remaining <= 0) {
    uint16_t offset_lines = -remaining;

    uint32_t offset = offset_lines * OFFSET_Y;
    memcpy(
      io_framebuffer->data, io_framebuffer->data + offset,
      OFFSET_Y * VGA_HEIGHT - offset
    );

    vga_rect(
      0, max_height - offset_lines, VGA_WIDTH - 1, max_height - 1, DEFAULT_BG, 0
    );

    cur_y -= offset_lines;
  }

  copy_to_main_fb();
  vga_set_framebuffer(NULL);
  draw_cursor();
  vga_present();
  vga_set_framebuffer(current_fb);
}

void io_setcursor(io_cursor_t cursor) { io_cursor_style = cursor; }

void io_movecursor(int32_t dx) {
  vga_font_ptr_t font = vga_getfont(io_text_font);

  dx *= font->char_width;
  if (dx < 0 && cur_x < -dx) {
    cur_x = 0;
  } else {
    cur_x += dx;
    if (cur_x >= VGA_WIDTH) nextline();
  }

  copy_to_main_fb();
  vga_framebuffer_t current_fb = vga_set_framebuffer(NULL);
  draw_cursor();
  vga_present();
  vga_set_framebuffer(current_fb);
}
