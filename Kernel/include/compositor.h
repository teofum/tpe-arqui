#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>
#include <types.h>

typedef enum {
  COMP_ABSOLUTE,
  COMP_RELATIVE,
} comp_move_mode_t;

void comp_init();

void comp_add(vga_framebuffer_t fb, point_t position, uint32_t order);

void comp_remove(vga_framebuffer_t fb);

void comp_top(vga_framebuffer_t fb);

void comp_reorder(vga_framebuffer_t fb, uint32_t order);

void comp_move(vga_framebuffer_t fb, point_t position, comp_move_mode_t mode);

void comp_composite();

#endif
