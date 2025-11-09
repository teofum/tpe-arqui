#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef int16_t pid_t;

typedef struct {
  int32_t x;
  int32_t y;
} point_t;

/*
 * Opaque framebuffer type.
 */
typedef struct vga_framebuffer_cdt_t *vga_framebuffer_t;

#endif
