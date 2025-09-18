#include <mem.h>
#include <vga.h>

/*
 * Internal implementation of framebuffer type.
 */
struct vga_framebuffer_cdt_t {
  uint32_t width;
  uint32_t height;
  uint8_t data[];
};

vga_framebuffer_t vga_create_framebuffer(int32_t width, int32_t height) {
  vbe_info_t info = vga_get_vbe_info();
  if (width <= 0) width += info.width;
  if (height <= 0) height += info.height;

  vga_framebuffer_t fb = mem_alloc(width * height * (info.bpp >> 3));
  fb->width = width;
  fb->height = height;

  return fb;
}
