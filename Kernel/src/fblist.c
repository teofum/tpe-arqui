#include "types.h"
#include <fblist.h>
#include <mem.h>
#include <stdint.h>

struct fblist_item_cdt_t {
  fb_entry_t data;
  struct fblist_item_cdt_t *tail;
};

struct fblist_cdt_t {
  fblist_item_t head;
};

fblist_t fblist_create() {
  fblist_t list = mem_alloc(sizeof(struct fblist_cdt_t));
  list->head = NULL;

  return list;
}

static fblist_item_t fblist_create_item(fb_entry_t data) {
  fblist_item_t item = mem_alloc(sizeof(fblist_item_t));
  item->data = data;
  item->tail = NULL;

  return item;
}

static fblist_item_t
fblist_add_after(fblist_item_t item, fblist_item_t new_item) {
  if (item->tail == NULL) {
    item->tail = new_item;
    return item;
  } else if (new_item->data.order < item->data.order) {
    new_item->tail = item;
    return new_item;
  } else {
    item->tail = fblist_add_after(item->tail, new_item);
    return item;
  }
}

void fblist_add(fblist_t list, fb_entry_t data) {
  fblist_item_t item = fblist_create_item(data);

  if (fblist_empty(list)) {
    list->head = item;
  } else {
    list->head = fblist_add_after(list->head, item);
  }
}

fb_entry_t fblist_pop(fblist_t list) {
  if (fblist_empty(list))
    return (fb_entry_t) {.fb = NULL, .offset = {.x = 0, .y = 0}};

  fblist_item_t head = list->head;
  fb_entry_t data = head->data;
  list->head = head->tail;
  mem_free(head);

  return data;
}

fb_entry_t fblist_peek(fblist_t list) {
  if (fblist_empty(list))
    return (fb_entry_t) {.fb = NULL, .offset = {.x = 0, .y = 0}};

  return list->head->data;
}

static fb_entry_t fblist_find_impl(fblist_item_t item, vga_framebuffer_t fb) {
  if (item->data.fb == fb)
    return (fb_entry_t) {.fb = NULL, .offset = {.x = 0, .y = 0}};
  if (item->tail == NULL)
    return (fb_entry_t) {.fb = NULL, .offset = {.x = 0, .y = 0}};
  return fblist_find_impl(item->tail, fb);
}

fb_entry_t fblist_find(fblist_t list, vga_framebuffer_t fb) {
  if (fblist_empty(list))
    return (fb_entry_t) {.fb = NULL, .offset = {.x = 0, .y = 0}};
  return fblist_find_impl(list->head, fb);
}

int fblist_empty(fblist_t list) { return list->head == NULL; }

static fblist_item_t
fblist_remove_impl(fblist_item_t item, vga_framebuffer_t fb, uint32_t *count) {
  if (item == NULL) return item;

  fblist_item_t tail = fblist_remove_impl(item->tail, fb, count);
  if (item->data.fb == fb) {
    (*count)++;
    mem_free(item);
    return tail;
  } else {
    item->tail = tail;
    return item;
  }
}

uint32_t fblist_remove_all(fblist_t list, vga_framebuffer_t fb) {
  if (fblist_empty(list)) return 0;

  uint32_t removed = 0;
  list->head = fblist_remove_impl(list->head, fb, &removed);
  return removed;
}

void fblist_top(fblist_t list, vga_framebuffer_t fb) {
  fb_entry_t entry = fblist_find(list, fb);
  if (entry.fb) {
    fblist_remove_all(list, fb);
    fblist_add(list, entry);
  }
}

static void fblist_destroy_impl(fblist_item_t item) {
  if (item->tail) fblist_destroy_impl(item->tail);
  mem_free(item);
}

void fblist_destroy(fblist_t list) {
  if (list->head) fblist_destroy_impl(list->head);
  mem_free(list);
}

fblist_item_t fblist_start(fblist_t list) { return list->head; }

fblist_item_t fblist_next(fblist_item_t item) { return item->tail; }

fb_entry_t fblist_get(fblist_item_t item) { return item->data; }
