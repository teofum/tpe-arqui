#include <graphics.h>
#include <mem.h>
#include <vga.h>

/*
 * Internal implementation of depthbuffer type.
 */
struct gfx_depthbuffer_cdt_t {
  uint32_t width;
  uint32_t height;
  float data[];
};

gfx_depthbuffer_t gfx_create_depthbuffer(int32_t width, int32_t height) {
  vbe_info_t info = vga_get_vbe_info();
  if (width <= 0) width += info.width;
  if (height <= 0) height += info.height;

  gfx_depthbuffer_t db = mem_alloc(width * height * sizeof(float) + 8);
  db->width = width;
  db->height = height;

  return db;
}
