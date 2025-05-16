#ifndef VGA_H
#define VGA_H
#include <stdint.h>

#define VGA_WIDTH 320
#define VGA_HEIGHT 200

#define VGA_VRAM_SIZE 64000

/*
 * VGA mode descriptor. Contains values for all the VGA registers that must
 * be set to switch the VGA device to a specific mode.
 */
typedef struct {
  uint8_t misc[1];
  uint8_t seq[5];
  uint8_t crtc[25];
  uint8_t gc[9];
  uint8_t ac[21];
} vga_mode_descriptor_t;

extern const vga_mode_descriptor_t *vga_g_320x200x256;

void vga_setMode(const vga_mode_descriptor_t *mode);
void vga_gfxMode();
void vga_textMode();

void vga_pixel(uint16_t x, uint16_t y, uint8_t color);

void vga_clear(uint8_t color);

#endif
