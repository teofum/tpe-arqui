#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
  int32_t x;
  int32_t y;
} point_t;

void mouse_init();

point_t mouse_getpos();

#endif
