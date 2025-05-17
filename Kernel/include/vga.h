#ifndef VGA_H
#define VGA_H
#include <stdint.h>

#define VGA_WIDTH 640
#define VGA_HEIGHT 480

#define VGA_VRAM_SIZE 64000
#define VGA_FONT_MAX_HEIGHT 24

/*
 * VGA mode descriptor. Contains values for all the VGA registers that must
 * be set to switch the VGA device to a specific mode.
 * Register values for VGA modes taken from
 * https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
 */
typedef struct {
  uint8_t misc[1];
  uint8_t seq[5];
  uint8_t crtc[25];
  uint8_t gc[9];
  uint8_t ac[21];
} vga_mode_descriptor_t;

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
 * Graphics mode, 640x480 16 color. Just as God intended.
 */
extern const vga_mode_descriptor_t *vga_g_640x480x16;

/*
 * Text mode, 80x25
 */
extern const vga_mode_descriptor_t *vga_t_80x25;

extern const vga_font_t *vga_defaultFont;
extern const vga_font_t *vga_comicsans;

/*
 * Switch VGA mode
 */
void vga_setMode(const vga_mode_descriptor_t *mode);

/*
 * Initialize the VGA driver in graphics mode.
 */
void vga_gfxMode();

/*
 * Initialize the graphics driver in text mode.
 * TODO: add text mode functionality
 */
void vga_textMode();

/*
 * Clear VRAM with a single solid color.
 */
void vga_pixel(uint16_t x, uint16_t y, uint8_t color);

/*
 * Plot a single pixel to VRAM.
 * This function does not do any bounds checking. The user should make sure to
 * only write to pixel coordinates within VRAM bounds.
 * Plotting individual pixels is quite slow; using provided driver functions
 * for primitives where available is recommended.
 */
void vga_clear(uint8_t color);

/*
 * Draw a line between two points.
 * Uses optimized drawing for horizontal and vertical lines, otherwise draws
 * with Bresenham's algorithm.
 */
void vga_line(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
);

/*
 * Draw a filled rectangle.
 */
void vga_rect(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
);

/*
 * Draw a stroked rectangle.
 */
void vga_frame(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
);

/*
 * Draw a shaded rectangle.
 */
void vga_shade(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
);

/*
 * Set the font used for drawing text.
 * Returns a pointer to the previous font used, so it can be restored with
 * another call to vga_font.
 */
const vga_font_t *vga_font(const vga_font_t *font);

void vga_text(uint16_t x, uint16_t y0, const char *string, uint8_t color);

#endif
