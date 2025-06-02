#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <fpmath.h>
#include <stdint.h>
#include <vga.h>

typedef enum {
  GFX_LIGHT_DIRECTIONAL,
  GFX_LIGHT_POINT,
} gfx_light_t;

typedef enum {
  GFX_LIGHT_POSITION,
  GFX_LIGHT_COLOR,
  GFX_AMBIENT_LIGHT,
} gfx_lightSetting_t;

typedef enum {
  GFX_MAT_MODEL,
  GFX_MAT_VIEW,
  GFX_MAT_PROJECTION,
} gfx_matrix_t;

typedef enum {
  GFX_RES_FULL,
  GFX_RES_HALF,
} gfx_res_t;

void gfx_clear(color_t color);

void gfx_drawPrimitives(
  float3 *vertices, float3 *normals, uint64_t n, float3 color
);

void gfx_drawPrimitivesIndexed(
  float3 *vertices, float3 *normals, uint32_t *indices, uint32_t *normalIndices,
  uint64_t n, float3 color
);

void gfx_setLight(gfx_lightSetting_t which, float3 *data);

void gfx_setLightType(gfx_light_t mode);

void gfx_setMatrix(gfx_matrix_t which, float4x4 *data);

void gfx_setRenderResolution(gfx_res_t res);

void gfx_present();

void gfx_parseObj(
  const char *data, float3 *v, float3 *n, uint32_t *vi, uint32_t *ni
);

#endif
