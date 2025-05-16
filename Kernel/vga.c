#include <vga.h>

#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_MISC_READ 0x3CC
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
/*			COLOR emulation		MONO emulation */
#define VGA_CRTC_INDEX 0x3D4 /* 0x3B4 */
#define VGA_CRTC_DATA 0x3D5  /* 0x3B5 */
#define VGA_INSTAT_READ 0x3DA

#define VGA_NUM_SEQ_REGS 5
#define VGA_NUM_CRTC_REGS 25
#define VGA_NUM_GC_REGS 9
#define VGA_NUM_AC_REGS 21
#define VGA_NUM_REGS                                                           \
  (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

uint8_t *vram = (uint8_t *) 0xA0000;

/*
 * Graphics mode, 640x480 16 color. Just as God intended.
 */
vga_mode_descriptor_t _vga_g_640x480x16 = {
  {0xE3},
  {0x03, 0x01, 0x08, 0x00, 0x06},
  {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3, 0xFF},
  {0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F, 0xFF},
  {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A,
   0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00}
};
const vga_mode_descriptor_t *vga_g_640x480x16 = &_vga_g_640x480x16;

/*
 * Text mode, 80x25
 */
vga_mode_descriptor_t _vga_t_80x25 = {
  {0x67},
  {0x03, 0x00, 0x03, 0x00, 0x02},
  {0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E, 0x00,
   0x00, 0x00, 0x50, 0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3, 0xFF},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF},
  {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A,
   0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00}
};
const vga_mode_descriptor_t *vga_t_80x25 = &_vga_t_80x25;

extern void _vga_setmode(const vga_mode_descriptor_t *mode);
extern void _vga_setplane(uint8_t plane);

/*
 * Mode switch
 */
void vga_setMode(const vga_mode_descriptor_t *mode) { _vga_setmode(mode); }

/*
 * Initialize the VGA driver in graphics mode.
 */
void vga_gfxMode() { vga_setMode(vga_g_640x480x16); }

/*
 * Initialize the graphics driver in text mode.
 * TODO: add text mode functionality
 */
void vga_textMode() { vga_setMode(vga_t_80x25); }

/*
 * Clear VRAM with a single solid color.
 */
void vga_clear(uint8_t color) {
  // Make a 64-bit dword so we can clear 8 bytes at a time
  // This is much faster than writing individual pixels
  uint64_t data[4] = {
    color & 0x1 ? 0xffffffffffffffff : 0x0,
    color & 0x2 ? 0xffffffffffffffff : 0x0,
    color & 0x4 ? 0xffffffffffffffff : 0x0,
    color & 0x8 ? 0xffffffffffffffff : 0x0,
  };

  // Fill VRAM with this color
  uint64_t *vram64 = (uint64_t *) vram;
  uint16_t end = (VGA_WIDTH * VGA_HEIGHT) >> 6;// vram size in bytes / 8

  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);
    for (uint16_t offset = 0; offset < end; offset++) vram64[offset] = data[p];
  }
}

/*
 * Plot a single pixel to VRAM.
 * This function does not do any bounds checking. The user should make sure to
 * only write to pixel coordinates within VRAM bounds.
 */
void vga_pixel(uint16_t x, uint16_t y, uint8_t color) {
  uint16_t offset = (x >> 3) + (VGA_WIDTH >> 3) * y;
  x &= 7;
  uint8_t mask = 0x80 >> x;
  uint8_t pmask = 1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);
    vram[offset] = pmask & color ? vram[offset] | mask : vram[offset] & ~mask;
    pmask <<= 1;
  }
}
