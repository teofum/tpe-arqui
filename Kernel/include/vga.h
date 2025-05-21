#ifndef VGA_H
#define VGA_H
#include <stdint.h>

#define VGA_FONT_MAX_HEIGHT 24

typedef uint32_t color_t;

/*
 * Font descriptor. Contains all relevant data about a font so the driver can
 * use it to draw text.
 */
typedef struct {
  /* Character data width, in pixels */
  uint8_t charWidth;

  /* Character data height, in pixels */
  uint8_t charHeight;

  /* Line height as drawn */
  uint8_t lineHeight;

  /* Space between characters in pixels */
  uint8_t spacing;

  /*
   * Character data. Stored linearly in memory: each character uses W * H bits,
   * where W and H are the font's character width and height respectively.
   * Characters are stored as a 1-bit per pixel bitmap in row-major order (left
   * to right, top to bottom).
   * Characters may be stored across multiple words. Data should be padded to a
   * multiple of 8 bytes.
   */
  const uint64_t *characterData;
} vga_font_t;


/*
 * Clear VRAM with a single solid color.
 */
void vga_clear(color_t color);

/*
 * Plot a single pixel to VRAM.
 * This function does not do any bounds checking. The user should make sure to
 * only write to pixel coordinates within VRAM bounds.
 * Plotting individual pixels is quite slow; using provided driver functions
 * for primitives where available is recommended.
 */
void vga_pixel(uint16_t x, uint16_t y, color_t color);

/*
 * Draw a line between two points.
 * Uses optimized drawing for horizontal and vertical lines, otherwise draws
 * with Bresenham's algorithm.
 */
void vga_line(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
);

/*
 * Draw a filled rectangle.
 */
void vga_rect(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
);

/*
 * Draw a stroked rectangle.
 */
void vga_frame(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
);

/*
 * Draw a shaded rectangle.
 */
void vga_shade(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
);

/*
 * Set the font used for drawing text.
 * Returns a pointer to the previous font used, so it can be restored with
 * another call to vga_font.
 */
const vga_font_t *vga_font(const vga_font_t *font);

typedef enum {
  VGA_TEXT_BG = 0x01,
} vga_textflags_t;

/*
 * Draw a line of text at a specific screen position.
 * This function does no bounds checking, the user must ensure the text doesn't
 * overflow the screen bounds.
 */
void vga_text(
  uint16_t x, uint16_t y0, const char *string, color_t color, uint8_t flags
);

#endif
