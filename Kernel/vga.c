#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>

#define VGA_WIDTH_CHUNKS 10

// Make a 64-bit dword so we can draw 8 bytes at a time
// This is much faster than writing individual pixels
#define color2data(color)                                                      \
  {                                                                            \
    (color) & 0x1 ? 0xffffffffffffffff : 0x0,                                  \
    (color) & 0x2 ? 0xffffffffffffffff : 0x0,                                  \
    (color) & 0x4 ? 0xffffffffffffffff : 0x0,                                  \
    (color) & 0x8 ? 0xffffffffffffffff : 0x0,                                  \
  }

#define reversebytes(lw)                                                       \
  (((lw & (0xffull << 0)) << 56) | ((lw & (0xffull << 8)) << 40) |             \
   ((lw & (0xffull << 16)) << 24) | ((lw & (0xffull << 24)) << 8) |            \
   ((lw & (0xffull << 32)) >> 8) | ((lw & (0xffull << 40)) >> 24) |            \
   ((lw & (0xffull << 48)) >> 40) | ((lw & (0xffull << 56)) >> 56))

// TODO maybe we should move this to a utils header?
#define abs(x) ((x) > 0 ? (x) : -(x))

/*
 * VRAM address in memory
 */
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

/*
 * Font data
 *
 */
uint64_t _vga_fontdata_dos_ank_8x16[190] = {
  0x0000000000000000, 0x0000000000000000, 0x0000101010101010,
  0x1000001038100000, 0x00006c6c24480000, 0x0000000000000000,
  0x00002424247e2424, 0x24247e2424240000, 0x0000103854545038,
  0x1454545438100000, 0x00002256542c0818, 0x1030246a4a440000,
  0x0000304848502066, 0x669494888c760000, 0x0000181808100000,
  0x0000000000000000, 0x0000081010202020, 0x2020201010080000,
  0x0000100808040404, 0x0404040808100000, 0x0000000010925438,
  0x3854921000000000, 0x00000000101010fe, 0x1010101000000000,
  0x0000000000000000, 0x0000000018180810, 0x000000000000007e,
  0x0000000000000000, 0x0000000000000000, 0x0000000018180000,
  0x0000020404080810, 0x1020204040800000, 0x0000182424424242,
  0x4242422424180000, 0x0000103050101010, 0x10101010107c0000,
  0x00003c4242020408, 0x10204042427e0000, 0x00003c424202021c,
  0x02024242423c0000, 0x0000040c0c142424, 0x4444fe04040e0000,
  0x00007e4040407c42, 0x02024242423c0000, 0x00003c424240407c,
  0x42424242423c0000, 0x00007e4242040408, 0x0810101010100000,
  0x00003c424242423c, 0x42424242423c0000, 0x00003c4242424242,
  0x3e020242423c0000, 0x0000000000001818, 0x0000000018180000,
  0x0000000000001818, 0x0000000018180810, 0x0000020408102040,
  0x2010080402000000, 0x0000000000007e00, 0x7e00000000000000,
  0x0000402010080402, 0x0408102040000000, 0x0000384482824408,
  0x1010001038100000, 0x00003c4242020232, 0x4a4a4a4a4a3c0000,
  0x0000182442424242, 0x7e42424242420000, 0x0000fc424242427c,
  0x4242424242fc0000, 0x00003a4642424040, 0x40404242423c0000,
  0x0000f84442424242, 0x4242424244f80000, 0x0000fe424040447c,
  0x4440404042fe0000, 0x0000fe424040447c, 0x4440404040e00000,
  0x00003a4642424040, 0x4e424242463a0000, 0x0000ee444444447c,
  0x4444444444ee0000, 0x00007c1010101010, 0x10101010107c0000,
  0x00003c0808080808, 0x0808080888700000, 0x0000ee4448506060,
  0x5050484844ee0000, 0x0000702020202020, 0x20202020227e0000,
  0x0000c6446c6c5454, 0x5444444444ee0000, 0x0000ee4464647454,
  0x5c4c4c4444ee0000, 0x00003c4242424242, 0x42424242423c0000,
  0x0000f84442424244, 0x7840404040e00000, 0x00003c4242424242,
  0x4242724a443a0000, 0x0000fc4242424478, 0x4c44444444ee0000,
  0x00003a4642402018, 0x04024242625c0000, 0x0000fe9292101010,
  0x1010101010380000, 0x0000ee4444444444, 0x4444444444380000,
  0x0000ee4444444444, 0x2828281010100000, 0x0000ee4444444454,
  0x547c6c6c44440000, 0x0000ee4444282810, 0x1028284444ee0000,
  0x0000ee4444442828, 0x1010101010380000, 0x0000fe8282040810,
  0x2040808282fe0000, 0x00001c1010101010, 0x10101010101c0000,
  0x0000181808100000, 0x0000000000000000, 0x0000380808080808,
  0x0808080808380000, 0x0000182442000000, 0x0000000000000000,
  0x0000000000000000, 0x000000000000ff00, 0x0000303020100000,
  0x0000000000000000, 0x0000000000003844, 0x043c44444c360000,
  0x0000c04040405864, 0x4242424264d80000, 0x0000000000003844,
  0x4440404444380000, 0x00000c040404344c, 0x444444444c360000,
  0x0000000000003844, 0x447c404444380000, 0x00000c1210107c10,
  0x1010101010380000, 0x0000000000003e44, 0x44444c3404444438,
  0x0000c04040405864, 0x4444444444ee0000, 0x0000001010003010,
  0x1010101010380000, 0x0000000808003808, 0x0808080808484830,
  0x0000c0404040404c, 0x4850605048ee0000, 0x0000301010101010,
  0x1010101010380000, 0x000000000000e854, 0x5454545454d60000,
  0x000000000000d864, 0x4444444444ee0000, 0x0000000000003c42,
  0x42424242423c0000, 0x0000000000007c22, 0x2222223c20202070,
  0x0000000000003e44, 0x4444443c0404040e, 0x0000000000006c32,
  0x2220202020700000, 0x0000000000003c42, 0x42300c42423c0000,
  0x0000001010107c10, 0x10101012120c0000, 0x000000000000cc44,
  0x444444444c360000, 0x000000000000ee44, 0x4444282810100000,
  0x000000000000c644, 0x445454546c440000, 0x000000000000c644,
  0x2810102844c60000, 0x000000000000ee44, 0x4424281808109060,
  0x0000000000007e42, 0x04081020427e0000, 0x00000c1010101020,
  0x10101010100c0000, 0x0000001010101010, 0x1010101010100000,
  0x0000300808080804, 0x0808080808300000, 0x0000324c00000000,
  0x0000000000000000
};

vga_font_t _vga_defaultFont = {
  .charWidth = 8,
  .charHeight = 16,
  .lineHeight = 16,
  .spacing = 1,
  .characterData = _vga_fontdata_dos_ank_8x16
};
const vga_font_t *vga_defaultFont = &_vga_defaultFont;

/*
 * Active font for text drawing
 */
vga_font_t *active_font = &_vga_defaultFont;

extern void _vga_setmode(const vga_mode_descriptor_t *mode);
extern void _vga_setplane(uint8_t plane);

void vga_setMode(const vga_mode_descriptor_t *mode) { _vga_setmode(mode); }

void vga_gfxMode() { vga_setMode(vga_g_640x480x16); }

void vga_textMode() { vga_setMode(vga_t_80x25); }

/*
 * Internal function for optimized rendering. Draws a 64-pixel "horizontal
 * chunk" to the currently selected VRAM plane.
 * @param x     initial x coordinate, in 64-pixel chunks
 * @param w     line width, in 64-pixel chunks
 * @param y     y coordinate, in pixels
 * @param chunk chunk data for the current plane (1bit per pixel)
 */
static inline void
vga_hchunk(uint16_t x, uint16_t w, uint16_t y, uint64_t chunk) {
  uint64_t *vram64 = (uint64_t *) vram;
  uint16_t offset = x + VGA_WIDTH_CHUNKS * y;

  for (uint16_t i = 0; i < w; i++) vram64[offset + i] = chunk;
}

/*
 * Similar to vga_hchunk, but with an additional mask parameter. Bits set to
 * zero in the mask are not drawn (VRAM contents are preserved).
 */
static inline void
vga_hchunkMasked(uint16_t x, uint16_t y, uint64_t chunk, uint64_t mask) {
  uint64_t *vram64 = (uint64_t *) vram;
  uint16_t offset = x + VGA_WIDTH_CHUNKS * y;

  vram64[offset] = (chunk & mask) | (vram64[offset] & ~mask);
}

/*
 * Optimized function to draw a horizontal line by chunks, this is very fast.
 */
static void vga_hline(uint16_t x0, uint16_t x1, uint16_t y, uint8_t color) {
  uint64_t data[4] = color2data(color);

  // Since we're dividing/moduloing by a power of two (64), we can use bitwise
  // ops to save a bunch of slow integer divisions
  uint16_t chunk0 = x0 >> 6;// x0 / 64
  uint16_t chunk1 = x1 >> 6;// x1 / 64

  uint64_t mask0 = 0xffffffffffffffff >> (x0 & 0x3f);
  uint64_t mask1 = 0xffffffffffffffff << (64 - ((x1 + 1) & 0x3f));

  // Handle special case where line is drawn entirely within a single chunk
  if (chunk0 == chunk1) mask0 &= mask1;

  // Reverse mask bytes for correct endianness (i hate little endian)
  mask0 = reversebytes(mask0);
  mask1 = reversebytes(mask1);

  uint16_t pchunk0, pchunk1;
  for (uint8_t p = 0; p < 4; p++) {
    pchunk0 = chunk0, pchunk1 = chunk1;
    _vga_setplane(p);

    // Draw first (partial) chunk
    vga_hchunkMasked(pchunk0, y, data[p], mask0);
    pchunk0++;

    if (pchunk1 >= pchunk0) {
      // Draw last (partial) chunk
      vga_hchunkMasked(pchunk1, y, data[p], mask1);
      pchunk1--;
    }

    if (pchunk1 >= pchunk0) {
      // Draw chunks in between
      vga_hchunk(pchunk0, pchunk1 - pchunk0 + 1, y, data[p]);
    }
  }
}

/*
 * Optimized function to draw a vertical line. Faster than running Bresenham's
 * algorithm, but not as fast as horizontal drawing.
 */
static void vga_vline(uint16_t x, uint16_t y0, uint16_t y1, uint8_t color) {
  uint16_t offset = (x >> 3) + (VGA_WIDTH >> 3) * y0;
  x &= 7;
  uint8_t mask = 0x80 >> x;
  uint8_t pmask = 1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);

    uint16_t poffset = offset;
    for (uint16_t y = y0; y <= y1; y++) {
      vram[poffset] =
        pmask & color ? vram[poffset] | mask : vram[poffset] & ~mask;
      poffset += VGA_WIDTH >> 3;
    }

    pmask <<= 1;
  }
}

/*
 * Bresenham's algorithm for |slope| < 1
 */
static void
vga_lineLo(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t yi = 1;

  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }

  uint8_t pmask = 1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);
    int16_t D = 2 * dy - dx;
    int16_t y = y0;

    for (int16_t x = x0; x <= x1; x++) {
      uint16_t offset = (x >> 3) + (VGA_WIDTH >> 3) * y;
      uint8_t mask = 0x80 >> (x & 7);
      vram[offset] = pmask & color ? vram[offset] | mask : vram[offset] & ~mask;

      if (D > 0) {
        y += yi;
        D += 2 * (dy - dx);
      } else {
        D += 2 * dy;
      }
    }

    pmask <<= 1;
  }
}

/*
 * Bresenham's algorithm for |slope| > 1
 */
static void
vga_lineHi(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color) {
  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t xi = 1;

  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }

  uint8_t pmask = 1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);
    int16_t D = 2 * dx - dy;
    int16_t x = x0;

    for (int16_t y = y0; y <= y1; y++) {
      uint16_t offset = (x >> 3) + (VGA_WIDTH >> 3) * y;
      uint8_t mask = 0x80 >> (x & 7);
      vram[offset] = pmask & color ? vram[offset] | mask : vram[offset] & ~mask;

      if (D > 0) {
        x += xi;
        D += 2 * (dx - dy);
      } else {
        D += 2 * dx;
      }
    }

    pmask <<= 1;
  }
}

void vga_line(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
) {
  if (y1 == y0) {
    if (x1 == x0) {
      vga_pixel(x0, y0, color);
    } else if (x1 > x0) {
      vga_hline(x0, x1, y0, color);
    } else {
      vga_hline(x1, x0, y0, color);
    }
  } else if (x1 == x0) {
    if (y1 > y0) {
      vga_vline(x0, y0, y1, color);
    } else {
      vga_vline(x0, y1, y0, color);
    }
  } else {
    if (abs(x1 - x0) > abs(y1 - y0)) {
      if (x1 > x0) {
        vga_lineLo(x0, y0, x1, y1, color);
      } else {
        vga_lineLo(x1, y1, x0, y0, color);
      }
    } else {
      if (y1 > y0) {
        vga_lineHi(x0, y0, x1, y1, color);
      } else {
        vga_lineHi(x1, y1, x0, y0, color);
      }
    }
  }
}

void vga_rect(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
) {
  // Very similar to vga_hline(), but we draw a bunch of horizontal lines at once
  // The reason we don't just call hline in a loop is so we don't have to do
  // the plane switching for each scanline
  uint64_t data[4] = color2data(color);

  uint16_t chunk0 = x0 >> 6;// x0 / 64
  uint16_t chunk1 = x1 >> 6;// x1 / 64

  uint64_t mask0 = 0xffffffffffffffff >> (x0 & 0x3f);
  uint64_t mask1 = 0xffffffffffffffff << (64 - ((x1 + 1) & 0x3f));

  if (chunk0 == chunk1) mask0 &= mask1;

  mask0 = reversebytes(mask0);
  mask1 = reversebytes(mask1);

  uint16_t pchunk0, pchunk1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);

    for (uint16_t y = y0; y <= y1; y++) {
      pchunk0 = chunk0, pchunk1 = chunk1;

      // Draw first (partial) chunk
      vga_hchunkMasked(pchunk0, y, data[p], mask0);
      pchunk0++;

      if (pchunk1 >= pchunk0) {
        // Draw last (partial) chunk
        vga_hchunkMasked(pchunk1, y, data[p], mask1);
        pchunk1--;
      }

      if (pchunk1 >= pchunk0) {
        // Draw chunks in between
        vga_hchunk(pchunk0, pchunk1 - pchunk0 + 1, y, data[p]);
      }
    }
  }
}

void vga_frame(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
) {
  uint64_t data[4] = color2data(color);

  uint16_t chunk0 = x0 >> 6;// x0 / 64
  uint16_t chunk1 = x1 >> 6;// x1 / 64

  uint64_t mask0 = 0xffffffffffffffff >> (x0 & 0x3f);
  uint64_t mask1 = 0xffffffffffffffff << (64 - ((x1 + 1) & 0x3f));

  if (chunk0 == chunk1) mask0 &= mask1;

  mask0 = reversebytes(mask0);
  mask1 = reversebytes(mask1);

  uint16_t pchunk0, pchunk1;
  uint8_t pmask = 1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);

    // Top and bottom lines
    pchunk0 = chunk0, pchunk1 = chunk1;

    // Draw first (partial) chunk
    vga_hchunkMasked(pchunk0, y0, data[p], mask0);
    vga_hchunkMasked(pchunk0, y1, data[p], mask0);
    pchunk0++;

    if (pchunk1 >= pchunk0) {
      // Draw last (partial) chunk
      vga_hchunkMasked(pchunk1, y0, data[p], mask1);
      vga_hchunkMasked(pchunk1, y1, data[p], mask1);
      pchunk1--;
    }

    if (pchunk1 >= pchunk0) {
      // Draw chunks in between
      vga_hchunk(pchunk0, pchunk1 - pchunk0 + 1, y0, data[p]);
      vga_hchunk(pchunk0, pchunk1 - pchunk0 + 1, y1, data[p]);
    }

    // Left and right lines
    uint16_t offset0 = (x0 >> 3) + (VGA_WIDTH >> 3) * y0;
    uint8_t mask0 = 0x80 >> (x0 & 7);
    uint16_t offset1 = (x1 >> 3) + (VGA_WIDTH >> 3) * y0;
    uint8_t mask1 = 0x80 >> (x1 & 7);

    for (uint16_t y = y0; y <= y1; y++) {
      vram[offset0] =
        pmask & color ? vram[offset0] | mask0 : vram[offset0] & ~mask0;
      vram[offset1] =
        pmask & color ? vram[offset1] | mask1 : vram[offset1] & ~mask1;

      offset0 += VGA_WIDTH >> 3;
      offset1 += VGA_WIDTH >> 3;
    }

    pmask <<= 1;
  }
}

void vga_shade(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color
) {
  uint64_t data[4] = color2data(color);

  uint16_t chunk0 = x0 >> 6;// x0 / 64
  uint16_t chunk1 = x1 >> 6;// x1 / 64

  uint64_t mask0 = 0xffffffffffffffff >> (x0 & 0x3f);
  uint64_t mask1 = 0xffffffffffffffff << (64 - ((x1 + 1) & 0x3f));

  if (chunk0 == chunk1) mask0 &= mask1;

  mask0 = reversebytes(mask0);
  mask1 = reversebytes(mask1);

  uint16_t pchunk0, pchunk1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);

    for (uint16_t y = y0; y <= y1; y++) {
      pchunk0 = chunk0, pchunk1 = chunk1;
      uint64_t rowmask = (y - y0) & 1 ? 0x5555555555555555 : 0xaaaaaaaaaaaaaaaa;

      // Draw first (partial) chunk
      vga_hchunkMasked(pchunk0, y, data[p], mask0 & rowmask);
      pchunk0++;

      if (pchunk1 >= pchunk0) {
        // Draw last (partial) chunk
        vga_hchunkMasked(pchunk1, y, data[p], mask1 & rowmask);
        pchunk1--;
      }

      for (uint16_t chunk = pchunk0; chunk <= pchunk1; chunk++) {
        // Draw chunks in between
        vga_hchunkMasked(chunk, y, data[p], rowmask);
      }
    }
  }
}

void vga_clear(uint8_t color) {
  uint64_t data[4] = color2data(color);

  // Fill VRAM with this color
  uint64_t *vram64 = (uint64_t *) vram;
  uint16_t end = (VGA_WIDTH * VGA_HEIGHT) >> 6;// vram size in bytes / 8

  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);
    for (uint16_t offset = 0; offset < end; offset++) vram64[offset] = data[p];
  }
}

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

#define ACTIVE_FONT_BITS (active_font->charWidth * active_font->charHeight)

void vga_char2masks(char c, uint64_t *masks, uint16_t offsetBits) {
  for (uint16_t y = 0; y < active_font->charHeight; y++) {
    for (uint16_t x0 = 0; x0 < active_font->charWidth; x0++) {
      size_t charOffsetBits =
        (c - ' ') * ACTIVE_FONT_BITS + y * active_font->charWidth + x0;
      size_t charOffsetWords = charOffsetBits >> 6;
      charOffsetBits &= 0x3f;

      uint16_t x = x0 + offsetBits;
      uint16_t xWord = x >> 6;
      x &= 0x3f;

      uint64_t charBit = (active_font->characterData[charOffsetWords] &
                          (1ull << (63 - charOffsetBits))) >>
                         (63 - charOffsetBits);

      uint16_t idx = y * VGA_WIDTH_CHUNKS + xWord;
      masks[idx] = masks[idx] | (charBit << (63 - x));
    }
  }
}

void vga_text(uint16_t x, uint16_t y0, const char *string, uint8_t color) {
  // First, calculate the size of the text to be drawn
  size_t len = 0;
  while (string[len] != 0) len++;
  if (!len) return;

  size_t drawWidth =
    len * active_font->charWidth + (len - 1) * active_font->spacing;
  size_t drawHeight = active_font->charHeight;

  // Allocate memory for the chunks and zero it
  uint64_t masks[VGA_WIDTH_CHUNKS * VGA_FONT_MAX_HEIGHT] = {0};

  uint16_t chunk0 = x >> 6;
  uint16_t chunk1 = (x + drawWidth) >> 6;
  size_t hchunks = chunk1 - chunk0 + 1;

  // Draw character bitmaps to masks
  uint16_t maskOffset = x & 0x3f;
  for (uint16_t i = 0; i < len; i++) {
    char c = string[i];
    vga_char2masks(
      c, masks,
      maskOffset + i * active_font->charWidth +
        (i == 0 ? 0 : i - 1) * active_font->spacing
    );
  }

  for (uint16_t i = 0; i < VGA_WIDTH_CHUNKS * VGA_FONT_MAX_HEIGHT; i++)
    masks[i] = reversebytes(masks[i]);

  uint16_t y1 = y0 + drawHeight;
  uint8_t pmask = 1;
  for (uint8_t p = 0; p < 4; p++) {
    _vga_setplane(p);

    for (uint16_t y = y0; y < y1; y++) {
      for (uint16_t ci = 0; ci < hchunks; ci++) {
        vga_hchunkMasked(
          chunk0 + ci, y, color & pmask ? 0xffffffffffffffff : 0x0,
          masks[(y - y0) * VGA_WIDTH_CHUNKS + ci]
        );
      }
    }
    pmask <<= 1;
  }
}
