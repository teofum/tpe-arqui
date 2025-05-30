#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <fpmath.h>
#include <stdint.h>
#include <vga.h>

void gfx_clear(color_t color);

void gfx_drawPrimitives(float3 *vertices, uint64_t n, float3 color);

void gfx_present();

#endif
