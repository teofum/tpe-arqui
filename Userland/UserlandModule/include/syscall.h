#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// TODO clean up this mess

typedef enum {
  /* I/O syscalls */
  SYS_READ = 0x03,
  SYS_WRITE = 0x04,
  SYS_WRITES = 0x07,
  SYS_PUTC = 0x08,
  SYS_CLEAR = 0x09,
  SYS_FONT = 0x0A,

  /* Keyboard syscalls */
  SYS_KBD_POLLEVENTS = 0x10,
  SYS_KBD_KEYDOWN = 0x11,
  SYS_KBD_KEYPRESSED = 0x12,
  SYS_KBD_KEYRELEASED = 0x13,
  SYS_KBD_GETKEYEVENT = 0x14,

  /* Video syscalls */
  SYS_VGA_CLEAR = 0x20,
  SYS_VGA_PIXEL = 0x21,
  SYS_VGA_LINE = 0x22,
  SYS_VGA_RECT = 0x23,
  SYS_VGA_FRAME = 0x24,
  SYS_VGA_SHADE = 0x25,
  SYS_VGA_GRADIENT = 0x26,
  SYS_VGA_FONT = 0x27,
  SYS_VGA_TEXT = 0x28,
  SYS_VGA_TEXTWRAP = 0x29,
  SYS_VGA_PRESENT = 0x2A,
  SYS_VGA_SET_FB = 0x2B,
} syscall_t;

typedef struct {
  uint8_t key;
  uint8_t isReleased;

  uint8_t shift : 1;
  uint8_t shift_r : 1;
  uint8_t ctrl : 1;
  uint8_t alt : 1;
  uint8_t capslock : 1;
} kbd_event_t;

typedef enum {
  IO_FONT_DEFAULT = 0,
  IO_FONT_TINY,
  IO_FONT_TINY_BOLD,
  IO_FONT_SMALL,
  IO_FONT_LARGE,
  IO_FONT_ALT,
  IO_FONT_ALT_BOLD,
  IO_FONT_FUTURE,
  IO_FONT_OLD,
} io_font_t;

extern uint64_t _syscall(uint64_t n, ...);

typedef uint32_t color_t;

typedef enum {
  VGA_TEXT_NORMAL = 0x00,// Draw text normally
  VGA_TEXT_BG = 0x01,    // Draw background color
  VGA_TEXT_NOFG = 0x02,  // Turn off foreground
  VGA_TEXT_INV = 0x03,   // Background + no foreground, "cutout" effect

  // Reuse some bits that are never used together
  VGA_GRAD_H = 0x00,// Horizontal gradient (left to right)
  VGA_GRAD_V = 0x01,// Vertical gradient (top to bottom)

  VGA_ALPHA_NONE = 0x00, // Disable alpha blending
  VGA_ALPHA_BLEND = 0x04,// Enable alpha blending

  VGA_WRAP_WORD = 0x08,// Wrap text by words instead of characters
} vga_drawflags_t;

extern void sys_vga_clear(color_t color);

/*
 * Plot a single pixel to VRAM.
 * This function does not do any bounds checking. The user should make sure to
 * only write to pixel coordinates within VRAM bounds.
 * Plotting individual pixels is quite slow; using provided driver functions
 * for primitives where available is recommended.
 */
extern void sys_vga_pixel(uint16_t x, uint16_t y, color_t color, uint8_t flags);

/*
 * Draw a line between two points.
 * Uses optimized drawing for horizontal and vertical lines, otherwise draws
 * with Bresenham's algorithm.
 */
extern void sys_vga_line(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
);

/*
 * Draw a filled rectangle.
 */
extern void sys_vga_rect(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
);

/*
 * Draw a stroked rectangle.
 */
extern void sys_vga_frame(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
);

/*
 * Draw a shaded rectangle.
 */
extern void sys_vga_shade(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
);

/*
 * Draw a rectangle filled with a gradient.
 * Set direction using the VGA_GRAD_X flags.
 */
extern void sys_vga_gradient(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint64_t colors,
  uint8_t flags
);

// /*
//  * Set the font used for drawing text.
//  * Returns a pointer to the previous font used, so it can be restored with
//  * another call to sys_vga_font.
//  */
// const vga_font_t *vga_font(const vga_font_t *font);
//
// /*
//  * Get the font used for drawing text
//  */
// const vga_font_t *vga_getfont();

/*
 * Draw a single character to VRAM at the specified position using the current
 * font.
 * This function does no bounds checking, use must ensure the character is in
 * screen bounds.
 */
extern void sys_vga_char(
  uint16_t x0, uint16_t y0, char c, color_t color, color_t bgColor,
  uint8_t flags
);

/*
 * Draw a line of text at a specific screen position using the current font.
 * Accepts newline characters, drawing multiple lines.
 * This function does no bounds checking, the user must ensure the text doesn't
 * overflow the screen bounds.
 */
extern void sys_vga_text(
  uint16_t x0, uint16_t y0, const char *string, color_t color, color_t bgColor,
  uint8_t flags
);

/*
 * Draw text starting at the specified position on screen, wrapping to a new
 * line if the text is longer, in pixels, than maxw. Set maxw to a negative
 * value to wrap at that offset from screen end.
 * This function does no bounds checking, the user must ensure the text doesn't
 * overflow the screen bounds.
 */
extern void sys_vga_textWrap(
  uint16_t x0, uint16_t y0, int16_t maxw, const char *string, uint64_t colors,
  uint8_t flags
);

extern void sys_vga_present();

#endif
