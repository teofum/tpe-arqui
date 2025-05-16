#include <stdint.h>
#include <stdlib.h>
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
