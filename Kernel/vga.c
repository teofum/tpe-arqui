#include <mem.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>

// TODO maybe we should move this to a utils header?
#define abs(x) ((x) > 0 ? (x) : -(x))

#define VGA_PHYSICAL_FRAMEBUFFER                                               \
  (vga_framebuffer_t)(uint64_t) vbe_mode_info->framebuffer
#define VGA_FRAMEBUFFER active_framebuffer

#define pixel_offset(x, y) ((x) * OFFSET_X + (y) * OFFSET_Y)

#define TAB_SIZE 8

#define b(c) ((c) & 0xff)
#define g(c) (((c) >> 8) & 0xff)
#define r(c) (((c) >> 16) & 0xff)

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define putpixel(fb, offset, color)                                            \
  fb->data[offset] = b(color), fb->data[offset + 1] = g(color),                \
  fb->data[offset + 2] = r(color)

#define ACTIVE_FONT vga_fonts[vga_active_font]

#define ACTIVE_FONT_BITS                                                       \
  ((((ACTIVE_FONT->char_width + 7) >> 3) << 3) * ACTIVE_FONT->char_height)

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

/*
 * Color modes for bitmap drawing.
 */
typedef enum {
  VGA_BMP_TRUECOLOR = 0x0,
  VGA_BMP_256,
  VGA_BMP_16,
} vga_colormode_t;

vbe_info_ptr vbe_mode_info = (vbe_info_ptr) 0x0000000000005C00;

/*
 * Internal implementation of framebuffer type.
 */
struct vga_framebuffer_cdt_t {
  uint32_t width;
  uint32_t height;
  uint8_t data[];
};

/*
 * Default framebuffer.
 * This is the main framebuffer used by the video driver, unless a different
 * one is requested.
 * Applications may wish to use a separate framebuffer, for example to preserve
 * its contents even if other things are drawn to the screen.
 */
static vga_framebuffer_t default_framebuffer;

/*
 * Pointer to the active framebuffer. This is the framebuffer being drawn to
 * and presented to the screen.
 * Applications that use their own framebuffer may either present it to the
 * screen directly, or copy it to the main framebuffer using vga_copy.
 */
static vga_framebuffer_t active_framebuffer;

/*
 * Font data
 */
#include <fontdata.h>

vga_font_data_t _vga_font_tiny = {
  .char_width = 6,
  .char_height = 8,
  .line_height = 8,
  .spacing = 0,
  .character_data = _vga_fontdata_hp_lx100_6x8
};

vga_font_data_t _vga_font_tiny_bold = {
  .char_width = 8,
  .char_height = 8,
  .line_height = 8,
  .spacing = 0,
  .character_data = _vga_fontdata_hp_lx100_8x8
};

vga_font_data_t _vga_font_small = {
  .char_width = 6,
  .char_height = 12,
  .line_height = 12,
  .spacing = 0,
  .character_data = _vga_fontdata_dos_ank_6x12
};

vga_font_data_t _vga_font_default = {
  .char_width = 8,
  .char_height = 16,
  .line_height = 16,
  .spacing = 0,
  .character_data = _vga_fontdata_dos_ank_8x16
};

vga_font_data_t _vga_font_large = {
  .char_width = 12,
  .char_height = 24,
  .line_height = 24,
  .spacing = 0,
  .character_data = _vga_fontdata_dos_ank_12x24
};

vga_font_data_t _vga_font_alt = {
  .char_width = 8,
  .char_height = 16,
  .line_height = 16,
  .spacing = 0,
  .character_data = _vga_fontdata_toshiba_txl2_8x16
};

vga_font_data_t _vga_font_alt_bold = {
  .char_width = 8,
  .char_height = 16,
  .line_height = 16,
  .spacing = 0,
  .character_data = _vga_fontdata_toshiba_txl1_8x16
};

vga_font_data_t _vga_font_future = {
  .char_width = 8,
  .char_height = 8,
  .line_height = 10,
  .spacing = 0,
  .character_data = _vga_fontdata_eagle2_8x8
};

vga_font_data_t _vga_font_old = {
  .char_width = 8,
  .char_height = 8,
  .line_height = 10,
  .spacing = 0,
  .character_data = _vga_fontdata_eagle3_8x8
};

const vga_font_data_t *vga_fonts[] = {
  &_vga_font_default,  &_vga_font_tiny,   &_vga_font_tiny_bold,
  &_vga_font_small,    &_vga_font_large,  &_vga_font_alt,
  &_vga_font_alt_bold, &_vga_font_future, &_vga_font_old,
};

/*
 * Active font for text drawing
 */
vga_font_t vga_active_font = VGA_FONT_DEFAULT;

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
static inline void blendpixel(
  vga_framebuffer_t fb, uint64_t offset, color_t color, uint8_t flags
) {
  if (flags & VGA_ALPHA_BLEND) {
    uint8_t *c = &fb->data[offset];
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
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X;

  uint64_t offset = pixel_offset(x0, y);
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
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_Y;

  uint64_t offset = pixel_offset(x, y0);
  for (uint16_t y = y0; y <= y1; y++) {
    blendpixel(fb, offset, color, flags);
    offset += step;
  }
}

/*
 * Bresenham's algorithm for |slope| < 1
 */
static void vga_line_lo(
  int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color, uint8_t flags
) {
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;

  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t yi = 1;

  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }

  int16_t D = 2 * dy - dx;
  int16_t y = y0;

  uint64_t offset = pixel_offset(x0, y0);
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
static void vga_line_hi(
  int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color, uint8_t flags
) {
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;

  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t xi = 1;

  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }

  int16_t D = 2 * dx - dy;
  int16_t x = x0;

  uint64_t offset = pixel_offset(x0, y0);
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

void vga_init() {
  default_framebuffer = vga_create_framebuffer(VGA_AUTO, VGA_AUTO);
  active_framebuffer = default_framebuffer;
}

vga_framebuffer_t vga_set_framebuffer(vga_framebuffer_t fb) {
  vga_framebuffer_t last = active_framebuffer;
  active_framebuffer = fb == NULL ? default_framebuffer : fb;

  return last == default_framebuffer ? NULL : last;
}

void vga_clear(color_t color) {
  uint64_t *fb = (uint64_t *) VGA_FRAMEBUFFER->data;
  uint64_t size = (OFFSET_Y >> 3) * vbe_mode_info->height;

  if (vbe_mode_info->bpp == 24) {
    uint64_t c = color & 0xffffff;
    uint64_t data[] = {
      (c << 48) | (c << 24) | c,
      (c << 56) | (c << 32) | (c << 8) | (c >> 16),
      (c << 40) | (c << 16) | (c >> 8),
    };

    for (uint64_t offset = 0; offset < size; offset += 3) {
      fb[offset + 0] = data[0];
      fb[offset + 1] = data[1];
      fb[offset + 2] = data[2];
    }
  } else {
    uint64_t data = color | ((uint64_t) color) << 32;
    for (uint64_t offset = 0; offset < size; offset++) { fb[offset] = data; }
  }
}

void vga_pixel(uint16_t x, uint16_t y, color_t color, uint8_t flags) {
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;
  uint64_t offset = pixel_offset(x, y);
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
        vga_line_lo(x0, y0, x1, y1, color, flags);
      } else {
        vga_line_lo(x1, y1, x0, y0, color, flags);
      }
    } else {
      if (y1 > y0) {
        vga_line_hi(x0, y0, x1, y1, color, flags);
      } else {
        vga_line_hi(x1, y1, x0, y0, color, flags);
      }
    }
  }
}

void vga_rect(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color_t color,
  uint8_t flags
) {
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X;

  for (uint16_t y = y0; y <= y1; y++) {
    uint64_t line_start = pixel_offset(x0, y);
    uint64_t line_end = pixel_offset(x1, y);

    for (uint64_t offset = line_start; offset <= line_end; offset += step) {
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
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X * 2;

  for (uint16_t y = y0; y <= y1; y++) {
    uint64_t line_start = pixel_offset(x0 + (y & 1), y);
    uint64_t line_end = pixel_offset(x1, y);

    for (uint64_t offset = line_start; offset <= line_end; offset += step) {
      blendpixel(fb, offset, color, flags);
    }
  }
}

void vga_gradient(
  uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint64_t colors,
  uint8_t flags
) {
  vga_framebuffer_t fb = VGA_FRAMEBUFFER;
  uint64_t step = OFFSET_X;

  color_t color1 = colors >> 32;
  color_t color2 = colors & 0xffffffff;

  for (uint16_t y = y0; y <= y1; y++) {
    uint64_t line_start = pixel_offset(x0, y);
    uint64_t line_end = pixel_offset(x1, y);

    for (uint64_t offset = line_start, x = x0; offset <= line_end;
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
  uint16_t x0, uint16_t y0, char c, color_t color, color_t bg_color,
  uint8_t flags
) {
  // Skip non printable characters
  if (c < ' ' || c == 0x7f) return;

  vga_framebuffer_t fb = VGA_FRAMEBUFFER;

  for (uint16_t y = 0; y < ACTIVE_FONT->char_height; y++) {
    uint64_t offset = pixel_offset(x0, y0 + y);

    for (uint16_t x = 0; x < ACTIVE_FONT->char_width; x++) {
      size_t char_offset_bits =
        (c - ' ') * ACTIVE_FONT_BITS +
        y * (((ACTIVE_FONT->char_width + 7) >> 3) << 3) + x;
      size_t char_offset_words = char_offset_bits >> 6;
      char_offset_bits &= 0x3f;

      uint64_t char_bit = (ACTIVE_FONT->character_data[char_offset_words] &
                           (1ull << (63 - char_offset_bits))) >>
                          (63 - char_offset_bits);

      if (char_bit && !(flags & VGA_TEXT_NOFG)) {
        blendpixel(fb, offset, color, flags);
      }
      if (!char_bit && (flags & VGA_TEXT_BG)) {
        blendpixel(fb, offset, bg_color, flags);
      }

      offset += OFFSET_X;
    }
  }
}

void vga_text(
  uint16_t x0, uint16_t y0, const char *string, color_t color, color_t bg_color,
  uint8_t flags
) {
  size_t i = 0;
  uint16_t x = x0, y = y0;
  uint16_t advance = ACTIVE_FONT->char_width + ACTIVE_FONT->spacing;
  char c;
  while ((c = string[i]) != 0) {
    if (c == '\n') {
      x = x0;
      y += ACTIVE_FONT->line_height;
    } else if (c == '\t') {
      x += advance * TAB_SIZE - (x % (advance * TAB_SIZE));
    } else {
      vga_char(x, y, c, color, bg_color, flags);
      x += advance;
    }

    i++;
  }
}

void vga_text_wrap(
  uint16_t x0, uint16_t y0, int16_t maxw, const char *str, uint64_t colors,
  uint8_t flags
) {
  uint16_t xmax = maxw < 0 ? maxw + vbe_mode_info->width : maxw + x0;

  color_t color = colors >> 32;
  color_t bg_color = colors & 0xffffffff;

  size_t i = 0;
  uint16_t x = x0, y = y0;
  uint16_t advance = ACTIVE_FONT->char_width + ACTIVE_FONT->spacing;
  char c;

  int wrap_next = 0;
  while ((c = str[i]) != 0) {
    if (flags & VGA_WRAP_WORD && wrap_next) {
      // Lookahead word, break if necessary
      uint16_t xend = x;
      char d;
      for (size_t j = i; (d = str[j]) != ' ' && d != '\n' && d != '\t'; j++) {
        xend += advance;
        if (xend > xmax) {
          x = x0;
          y += ACTIVE_FONT->line_height;
          break;
        }
      }
    }

    if (c == '\n') {
      x = x0;
      y += ACTIVE_FONT->line_height;
      wrap_next = 1;
    } else if (c == '\t') {
      x += advance * TAB_SIZE - (x % (advance * TAB_SIZE));
      wrap_next = 1;
    } else if (c != ' ' || x > x0) {
      vga_char(x, y, c, color, bg_color, flags);
      x += advance;
      wrap_next = c == ' ';

      if (x >= xmax) {
        x = x0;
        y += ACTIVE_FONT->line_height;
        wrap_next = 0;
      }
    } else {
      wrap_next = 0;
    }

    i++;
  }
}

static inline void
putpixels(uint64_t offset, uint16_t scale, color_t color, uint8_t flags) {
  for (int i = 0; i < scale; i++) {
    for (int j = 0; j < scale; j++) {
      blendpixel(
        VGA_FRAMEBUFFER, offset + i * OFFSET_Y + j * OFFSET_X, color, flags
      );
    }
  }
}

void vga_bitmap(
  uint16_t x0, uint16_t y0, uint8_t *data, uint16_t scale, uint8_t flags
) {
  // First 8 bytes of bitmap data are a header, with 4 bytes for width and height each
  uint32_t width = *(uint32_t *) data;
  data += 4;
  uint32_t height = *(uint32_t *) data;
  data += 4;

  vga_colormode_t colormode = *(uint32_t *) data;
  data += 4;
  uint32_t palette_size =
    colormode == VGA_BMP_TRUECOLOR ? 0 : (colormode == VGA_BMP_256 ? 256 : 16);

  color_t palette[256];
  for (uint32_t i = 0; i < palette_size; i++) {
    palette[i] = *(uint32_t *) data;
    data += 4;
  }

  uint32_t y1 = y0 + height;
  uint32_t x1 = x0 + width;

  uint32_t i = 0;
  uint64_t offset = pixel_offset(x0, y0);
  uint64_t line_start = offset;

  uint64_t x_offset = OFFSET_X * scale;
  uint64_t y_offset = OFFSET_Y * scale;

  switch (colormode) {
    case VGA_BMP_TRUECOLOR:
      for (uint16_t y = y0; y < y1; y++) {
        for (uint16_t x = x0; x < x1; x++) {
          // Offset data pointer left by one byte so we get RGB data, alpha is thrown away.
          putpixels(offset, scale, *(uint32_t *) (data - 1), flags);
          offset += x_offset;
          data += 3;
        }

        line_start += y_offset;
        offset = line_start;
      }
      break;
    case VGA_BMP_256:
      for (uint16_t y = y0; y < y1; y++) {
        for (uint16_t x = x0; x < x1; x++) {
          color_t color = palette[*data++];
          putpixels(offset, scale, color, flags);
          offset += x_offset;
        }

        line_start += y_offset;
        offset = line_start;
      }
      break;
    case VGA_BMP_16:
      for (uint16_t y = y0; y < y1; y++) {
        for (uint16_t x = x0; x < x1; x++) {
          uint8_t idx = data[i >> 1];
          idx = (i % 2) ? idx & 0x0f : idx >> 4;

          color_t color = palette[idx];
          putpixels(offset, scale, color, flags);
          offset += x_offset;
          i++;
        }

        line_start += y_offset;
        offset = line_start;
      }
      break;
  }
}

vga_font_t vga_font(vga_font_t font) {
  vga_font_t last_font = vga_active_font;
  vga_active_font = font;

  return last_font;
}

vga_font_ptr_t vga_getfont(vga_font_t font) { return vga_fonts[font]; }

/*
 * 64-bit aligned memcpy, super fast
 */
static void memcpy64(uint64_t *dst, uint64_t *src, uint64_t len) {
  for (uint64_t i = 0; i < len; i++) { *dst++ = *src++; }
}

void vga_present() {
  memcpy64(
    (uint64_t *) VGA_PHYSICAL_FRAMEBUFFER, (uint64_t *) VGA_FRAMEBUFFER->data,
    VGA_HEIGHT * (OFFSET_Y >> 3)
  );
}

void vga_copy(vga_framebuffer_t dst, vga_framebuffer_t src, uint32_t offset) {
  if (dst == NULL) dst = default_framebuffer;
  if (src == NULL) src = default_framebuffer;

  memcpy64(
    (uint64_t *) (dst->data + offset * OFFSET_Y), (uint64_t *) src->data,
    (OFFSET_Y >> 3) * (VGA_HEIGHT - offset)
  );
}

void vga_copy2x(vga_framebuffer_t dst_fb, vga_framebuffer_t src_fb) {
  if (dst_fb == NULL) dst_fb = default_framebuffer;
  if (src_fb == NULL) src_fb = default_framebuffer;

  uint8_t *src = src_fb->data;
  uint8_t *dst1 = dst_fb->data;
  uint8_t *dst2 = dst_fb->data + OFFSET_Y;
  uint64_t width = VGA_WIDTH >> 1;
  uint64_t height = VGA_HEIGHT >> 1;
  for (uint64_t y = 0; y < height; y++) {
    for (uint64_t x = 0; x < width; x++) {
      dst1[0] = dst1[OFFSET_X + 0] = dst2[0] = dst2[OFFSET_X + 0] = src[0];
      dst1[1] = dst1[OFFSET_X + 1] = dst2[1] = dst2[OFFSET_X + 1] = src[1];
      dst1[2] = dst1[OFFSET_X + 2] = dst2[2] = dst2[OFFSET_X + 2] = src[2];

      src += OFFSET_X;
      dst1 += (OFFSET_X << 1);
      dst2 += (OFFSET_X << 1);
    }
    src += (OFFSET_Y >> 1);
    dst1 += OFFSET_Y;
    dst2 += OFFSET_Y;
  }
}

vbe_info_t vga_get_vbe_info() { return *vbe_mode_info; }

vga_framebuffer_t vga_create_framebuffer(int32_t width, int32_t height) {
  if (width <= 0) width += VGA_WIDTH;
  if (height <= 0) height += VGA_HEIGHT;

  vga_framebuffer_t fb = mem_alloc(width * height * OFFSET_X);
  fb->width = width;
  fb->height = height;

  return fb;
}
