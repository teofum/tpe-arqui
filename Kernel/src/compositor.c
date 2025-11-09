#include <compositor.h>
#include <fblist.h>
#include <process.h>
#include <stddef.h>
#include <vga.h>

static fblist_t fblist;

void comp_init() { fblist = fblist_create(); }

void comp_add(vga_framebuffer_t fb, point_t position, uint32_t order) {
  fblist_add(
    fblist, (fb_entry_t) {.fb = fb, .offset = position, .order = order}
  );
}

void comp_remove(vga_framebuffer_t fb) { fblist_remove_all(fblist, fb); }

void comp_top(vga_framebuffer_t fb) { fblist_top(fblist, fb); }

void comp_reorder(vga_framebuffer_t fb, uint32_t order) {
  fb_entry_t entry = fblist_find(fblist, fb);
  if (!entry.fb) return;

  entry.order = order;
  fblist_remove_all(fblist, fb);
  fblist_add(fblist, entry);
}

void comp_move(vga_framebuffer_t fb, point_t position, comp_move_mode_t mode) {
  fb_entry_t entry = fblist_find(fblist, fb);
  if (!entry.fb) return;

  entry.offset.x =
    mode == COMP_ABSOLUTE ? position.x : entry.offset.x + position.x;
  entry.offset.y =
    mode == COMP_ABSOLUTE ? position.y : entry.offset.y + position.y;
  fblist_remove_all(fblist, fb);
  fblist_add(fblist, entry);
}

void comp_composite() {
  fblist_item_t item = fblist_start(fblist);

  uint32_t fb_handle = proc_set_framebuffer(FB_DEFAULT);
  vga_clear(0x000000);
  proc_set_framebuffer(fb_handle);

  while (item != NULL) {
    fb_entry_t entry = fblist_get(item);

    vga_copy_ex(
      NULL, entry.fb,
      (vga_copy_ex_opts_t) {
        .scale = 1,
        .sx = 0,
        .sy = 0,
        .sh = 0,
        .sw = 0,
        .dx = entry.offset.x,
        .dy = entry.offset.y,
      }
    );

    item = fblist_next(item);
  }

  vga_present();
}
