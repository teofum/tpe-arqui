#ifndef VGA_H
#define VGA_H
#include <stdint.h>

#define VGA_WIDTH 640
#define VGA_HEIGHT 480

#define VGA_VRAM_SIZE 64000

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
 * Graphics mode, 640x480 16 color. Just as God intended.
 */
extern const vga_mode_descriptor_t *vga_g_640x480x16;

/*
 * Text mode, 80x25
 */
extern const vga_mode_descriptor_t *vga_t_80x25;

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

#endif
