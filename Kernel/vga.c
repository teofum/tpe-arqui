#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>

// TODO maybe we should move this to a utils header?
#define abs(x) ((x) > 0 ? (x) : -(x))

#define VGA_FRAMEBUFFER (uint8_t *) (uint64_t) VBE_mode_info->framebuffer

#define OFFSET_X (VBE_mode_info->bpp >> 3)
#define OFFSET_Y (VBE_mode_info->pitch)
#define pixelOffset(x, y) ((x) * OFFSET_X + (y) * OFFSET_Y)

#define TAB_SIZE 8

#define b(c) ((c) & 0xff)
#define g(c) (((c) >> 8) & 0xff)
#define r(c) (((c) >> 16) & 0xff)

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define putpixel(fb, offset, color)                                            \
  fb[offset] = b(color), fb[offset + 1] = g(color), fb[offset + 2] = r(color)

#define ACTIVE_FONT_BITS (active_font->charWidth * active_font->charHeight)

VBEInfoPtr VBE_mode_info = (VBEInfoPtr) 0x0000000000005C00;

/*
 * Font data
 */
#include <fontdata.h>

vga_font_t _vga_defaultFont = {
  .charWidth = 8,
  .charHeight = 16,
  .lineHeight = 16,
  .spacing = 0,
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
 * Alpha premultiply using evil bit manipulation tricks.
 * Ref: https://arxiv.org/pdf/2202.02864
 */
static inline color_t fast_premul(color_t color) {
  uint32_t alpha = color >> 24;

  color |= 0xff000000;
  uint64_t rbga =
    ((uint64_t) (color & 0x00ff00ff) << 32) | ((color >> 8) & 0x00ff00ff);

  rbga *= alpha;
  rbga += 0x0080008000800080;
  rbga += (rbga >> 8) & 0x00ff00ff00ff00ff;
  rbga &= 0xff00ff00ff00ff00;

  return rbga | (rbga >> 40);
}

/*
 * Plot a single pixel into VRAM, alpha blending with the background if the
 * alpha blending flag is set.
 */
static inline void
blendpixel(uint8_t *fb, uint64_t offset, color_t color, uint8_t flags) {
  if (flags & VGA_ALPHA_BLEND) {
    uint8_t *c = &fb[offset];
    uint32_t current = rgba(c[2], c[1], c[0], 0xff - (color >> 24));

    uint32_t blended = fast_premul(current) + fast_premul(color);
    putpixel(fb, offset, blended);
  } else {
    putpixel(fb, offset, color);
  }
}

/*
 * Optimized function to draw a horizontal line.
 */
static void
vga_hline(uint16_t x0, uint16_t x1, uint16_t y, color_t color, uint8_t flags) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X;

  uint64_t offset = pixelOffset(x0, y);
  for (uint16_t x = x0; x <= x1; x++) {
    blendpixel(fb, offset, color, flags);
    offset += step;
  }
}

/*
 * Optimized function to draw a vertical line.
 */
static void
vga_vline(uint16_t x, uint16_t y0, uint16_t y1, color_t color, uint8_t flags) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_Y;

  uint64_t offset = pixelOffset(x, y0);
  for (uint16_t y = y0; y <= y1; y++) {
    blendpixel(fb, offset, color, flags);
    offset += step;
  }
}

/*
 * Bresenham's algorithm for |slope| < 1
 */
static void vga_lineLo(
  int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color, uint8_t flags
) {
  uint8_t *fb = VGA_FRAMEBUFFER;

  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t yi = 1;

  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }

  int16_t D = 2 * dy - dx;
  int16_t y = y0;

  uint64_t offset = pixelOffset(x0, y0);
  for (int16_t x = x0; x <= x1; x++) {
    blendpixel(fb, offset, color, flags);

    offset += OFFSET_X;
    if (D > 0) {
      y += yi;
      offset += OFFSET_Y * yi;
      D += 2 * (dy - dx);
    } else {
      D += 2 * dy;
    }
  }
}

/*
 * Bresenham's algorithm for |slope| > 1
 */
static void vga_lineHi(
  int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color, uint8_t flags
) {
  uint8_t *fb = VGA_FRAMEBUFFER;

  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t xi = 1;

  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }

  int16_t D = 2 * dx - dy;
  int16_t x = x0;

  uint64_t offset = pixelOffset(x0, y0);
  for (int16_t y = y0; y <= y1; y++) {
    blendpixel(fb, offset, color, flags);

    offset += OFFSET_Y;
    if (D > 0) {
      x += xi;
      offset += OFFSET_X * xi;
      D += 2 * (dx - dy);
    } else {
      D += 2 * dx;
    }
  }
}

void vga_clear(color_t color) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t offset = 0;
  uint64_t size = OFFSET_Y * VBE_mode_info->height;
  uint64_t step = OFFSET_X;

  for (; offset < size; offset += step) putpixel(fb, offset, color);
}

void vga_pixel(uint16_t x, uint16_t y, color_t color, uint8_t flags) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t offset = pixelOffset(x, y);
  blendpixel(fb, offset, color, flags);
}

void vga_line(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
) {
  if (y1 == y0) {
    if (x1 == x0) {
      vga_pixel(x0, y0, color, flags);
    } else if (x1 > x0) {
      vga_hline(x0, x1, y0, color, flags);
    } else {
      vga_hline(x1, x0, y0, color, flags);
    }
  } else if (x1 == x0) {
    if (y1 > y0) {
      vga_vline(x0, y0, y1, color, flags);
    } else {
      vga_vline(x0, y1, y0, color, flags);
    }
  } else {
    if (abs(x1 - x0) > abs(y1 - y0)) {
      if (x1 > x0) {
        vga_lineLo(x0, y0, x1, y1, color, flags);
      } else {
        vga_lineLo(x1, y1, x0, y0, color, flags);
      }
    } else {
      if (y1 > y0) {
        vga_lineHi(x0, y0, x1, y1, color, flags);
      } else {
        vga_lineHi(x1, y1, x0, y0, color, flags);
      }
    }
  }
}

void vga_rect(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X;

  for (uint16_t y = y0; y <= y1; y++) {
    uint64_t lineStart = pixelOffset(x0, y);
    uint64_t lineEnd = pixelOffset(x1, y);

    for (uint64_t offset = lineStart; offset <= lineEnd; offset += step) {
      blendpixel(fb, offset, color, flags);
    }
  }
}

void vga_frame(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
) {
  vga_hline(x0, x1, y0, color, flags);
  vga_hline(x0, x1, y1, color, flags);
  vga_vline(x0, y0, y1, color, flags);
  vga_vline(x1, y0, y1, color, flags);
}

void vga_shade(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X * 2;

  for (uint16_t y = y0; y <= y1; y++) {
    uint64_t lineStart = pixelOffset(x0 + (y & 1), y);
    uint64_t lineEnd = pixelOffset(x1, y);

    for (uint64_t offset = lineStart; offset <= lineEnd; offset += step) {
      blendpixel(fb, offset, color, flags);
    }
  }
}

void vga_gradient(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color1,
  color_t color2, uint8_t flags
) {
  uint8_t *fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X;

  for (uint16_t y = y0; y <= y1; y++) {
    uint64_t lineStart = pixelOffset(x0, y);
    uint64_t lineEnd = pixelOffset(x1, y);

    for (uint64_t offset = lineStart, x = x0; offset <= lineEnd;
         offset += step, x++) {
      uint32_t t = (flags & VGA_GRAD_V) ? (y - y0) * 0xff / (y1 - y0)
                                        : (x - x0) * 0xff / (x1 - x0);

      color1 = (color1 & 0xffffff) | ((0xff - t) << 24);
      color2 = (color2 & 0xffffff) | (t << 24);
      color_t color = fast_premul(color1) + fast_premul(color2);

      blendpixel(fb, offset, color, flags);
    }
  }
}

void vga_char(
  uint16_t x0, uint16_t y0, char c, color_t color, color_t bgColor,
  uint8_t flags
) {
  uint8_t *fb = VGA_FRAMEBUFFER;

  for (uint16_t y = 0; y < active_font->charHeight; y++) {
    uint64_t offset = pixelOffset(x0, y0 + y);

    for (uint16_t x = 0; x < active_font->charWidth; x++) {
      size_t charOffsetBits =
        (c - ' ') * ACTIVE_FONT_BITS + y * active_font->charWidth + x;
      size_t charOffsetWords = charOffsetBits >> 6;
      charOffsetBits &= 0x3f;

      uint64_t charBit = (active_font->characterData[charOffsetWords] &
                          (1ull << (63 - charOffsetBits))) >>
                         (63 - charOffsetBits);

      if (charBit && !(flags & VGA_TEXT_NOFG)) {
        blendpixel(fb, offset, color, flags);
      }
      if (!charBit && (flags & VGA_TEXT_BG)) {
        blendpixel(fb, offset, bgColor, flags);
      }

      offset += OFFSET_X;
    }
  }
}

void vga_text(
  uint16_t x0, uint16_t y0, const char *string, color_t color, color_t bgColor,
  uint8_t flags
) {
  size_t i = 0;
  uint16_t x = x0, y = y0;
  uint16_t advance = active_font->charWidth + active_font->spacing;
  char c;
  while ((c = string[i]) != 0) {
    if (c == '\n') {
      x = x0;
      y += active_font->lineHeight;
    } else if (c == '\t') {
      x += advance * TAB_SIZE - (x % (advance * TAB_SIZE));
    } else {
      vga_char(x, y, c, color, bgColor, flags);
      x += advance;
    }

    i++;
  }
}

void vga_textWrap(
  uint16_t x0, uint16_t y0, int16_t maxw, const char *str, color_t color,
  color_t bgColor, uint8_t flags
) {
  uint16_t xmax = maxw < 0 ? maxw + VBE_mode_info->width : maxw + x0;

  size_t i = 0;
  uint16_t x = x0, y = y0;
  uint16_t advance = active_font->charWidth + active_font->spacing;
  char c;

  int wrapNext = 0;
  while ((c = str[i]) != 0) {
    if (flags & VGA_WRAP_WORD && wrapNext) {
      // Lookahead word, break if necessary
      uint16_t xend = x;
      char d;
      for (size_t j = i; (d = str[j]) != ' ' && d != '\n' && d != '\t'; j++) {
        xend += advance;
        if (xend > xmax) {
          x = x0;
          y += active_font->lineHeight;
          break;
        }
      }
    }

    if (c == '\n') {
      x = x0;
      y += active_font->lineHeight;
      wrapNext = 1;
    } else if (c == '\t') {
      x += advance * TAB_SIZE - (x % (advance * TAB_SIZE));
      wrapNext = 1;
    } else if (c != ' ' || x > x0) {
      vga_char(x, y, c, color, bgColor, flags);
      x += advance;
      wrapNext = c == ' ';

      if (x >= xmax) {
        x = x0;
        y += active_font->lineHeight;
        wrapNext = 0;
      }
    } else {
      wrapNext = 0;
    }

    i++;
  }
}

const vga_font_t *vga_font(const vga_font_t *font) {
  const vga_font_t *lastFont = active_font;
  active_font = font;
  return lastFont;
}
