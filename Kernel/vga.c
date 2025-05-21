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

struct vbe_mode_info_structure {
  uint16_t
    attributes;// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
  uint8_t window_a;    // deprecated
  uint8_t window_b;    // deprecated
  uint16_t granularity;// deprecated; used while calculating bank numbers
  uint16_t window_size;
  uint16_t segment_a;
  uint16_t segment_b;
  uint32_t
    win_func_ptr;// deprecated; used to switch banks from protected mode without returning to real mode
  uint16_t pitch;// number of bytes per horizontal line
  uint16_t width;// width in pixels
  uint16_t height;// height in pixels
  uint8_t w_char; // unused...
  uint8_t y_char; // ...
  uint8_t planes;
  uint8_t bpp;  // bits per pixel in this mode
  uint8_t banks;// deprecated; total number of banks in this mode
  uint8_t memory_model;
  uint8_t
    bank_size;// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
  uint8_t image_pages;
  uint8_t reserved0;

  uint8_t red_mask;
  uint8_t red_position;
  uint8_t green_mask;
  uint8_t green_position;
  uint8_t blue_mask;
  uint8_t blue_position;
  uint8_t reserved_mask;
  uint8_t reserved_position;
  uint8_t direct_color_attributes;

  uint32_t
    framebuffer;// physical address of the linear frame buffer; write here to draw to the screen
  uint32_t off_screen_mem_off;
  uint16_t
    off_screen_mem_size;// size of memory in the framebuffer but not being displayed on the screen
  uint8_t reserved1[206];
} __attribute__((packed));

typedef struct vbe_mode_info_structure *VBEInfoPtr;

VBEInfoPtr VBE_mode_info = (VBEInfoPtr) 0x0000000000005C00;

/*
 * Font data
 */
#include <fontdata.h>

vga_font_t _vga_defaultFont = {
  .charWidth = 8,
  .charHeight = 16,
  .lineHeight = 16,
  .spacing = 1,
  .characterData = _vga_fontdata_dos_ank_8x16
};
const vga_font_t *vga_defaultFont = &_vga_defaultFont;

vga_font_t _vga_comicsans = {
  .charWidth = 16,
  .charHeight = 24,
  .lineHeight = 24,
  .spacing = 0,
  .characterData = _vga_fontdata_comicsans
};
const vga_font_t *vga_comicsans = &_vga_comicsans;

/*
 * Active font for text drawing
 */
const vga_font_t *active_font = &_vga_defaultFont;

/*
 * Optimized function to draw a horizontal line by chunks, this is very fast.
 */
static void vga_hline(uint16_t x0, uint16_t x1, uint16_t y, color_t color) {}

/*
 * Optimized function to draw a vertical line. Faster than running Bresenham's
 * algorithm, but not as fast as horizontal drawing.
 */
static void vga_vline(uint16_t x, uint16_t y0, uint16_t y1, color_t color) {}

/*
 * Bresenham's algorithm for |slope| < 1
 */
static void
vga_lineLo(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color) {
  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t yi = 1;

  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }

  // uint8_t pmask = 1;
  // for (uint8_t p = 0; p < 4; p++) {
  //   _vga_setplane(p);
  //   int16_t D = 2 * dy - dx;
  //   int16_t y = y0;
  //
  //   for (int16_t x = x0; x <= x1; x++) {
  //     uint16_t offset = (x >> 3) + (VGA_WIDTH >> 3) * y;
  //     uint8_t mask = 0x80 >> (x & 7);
  //     vram[offset] = pmask & color ? vram[offset] | mask : vram[offset] & ~mask;
  //
  //     if (D > 0) {
  //       y += yi;
  //       D += 2 * (dy - dx);
  //     } else {
  //       D += 2 * dy;
  //     }
  //   }
  //
  //   pmask <<= 1;
  // }
}

/*
 * Bresenham's algorithm for |slope| > 1
 */
static void
vga_lineHi(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color) {
  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t xi = 1;

  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }

  // uint8_t pmask = 1;
  // for (uint8_t p = 0; p < 4; p++) {
  //   _vga_setplane(p);
  //   int16_t D = 2 * dx - dy;
  //   int16_t x = x0;
  //
  //   for (int16_t y = y0; y <= y1; y++) {
  //     uint16_t offset = (x >> 3) + (VGA_WIDTH >> 3) * y;
  //     uint8_t mask = 0x80 >> (x & 7);
  //     vram[offset] = pmask & color ? vram[offset] | mask : vram[offset] & ~mask;
  //
  //     if (D > 0) {
  //       x += xi;
  //       D += 2 * (dx - dy);
  //     } else {
  //       D += 2 * dx;
  //     }
  //   }
  //
  //   pmask <<= 1;
  // }
}

void vga_line(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
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
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
) {}

void vga_frame(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
) {}

void vga_shade(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color
) {}

void vga_clear(color_t color) {}

void vga_pixel(uint16_t x, uint16_t y, color_t color) {}

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

void vga_text(
  uint16_t x, uint16_t y0, const char *string, color_t color, uint8_t flags
) {}

const vga_font_t *vga_font(const vga_font_t *font) {
  const vga_font_t *lastFont = active_font;
  active_font = font;
  return lastFont;
}
