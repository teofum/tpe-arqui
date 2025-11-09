#ifndef FBLIST_H
#define FBLIST_H

#include <stdint.h>
#include <types.h>

typedef struct {
  vga_framebuffer_t fb;
  uint32_t order;
  point_t offset;
} fb_entry_t;

typedef struct fblist_cdt_t *fblist_t;
typedef struct fblist_item_cdt_t *fblist_item_t;

fblist_t fblist_create();

void fblist_add(fblist_t list, fb_entry_t data);

fb_entry_t fblist_pop(fblist_t list);

fb_entry_t fblist_find(fblist_t list, vga_framebuffer_t fb);

int fblist_empty(fblist_t list);

uint32_t fblist_remove_all(fblist_t list, vga_framebuffer_t fb);

void fblist_top(fblist_t list, vga_framebuffer_t fb);

void fblist_destroy(fblist_t list);

fblist_item_t fblist_start(fblist_t list);

fblist_item_t fblist_next(fblist_item_t item);

fb_entry_t fblist_get(fblist_item_t item);

#endif
