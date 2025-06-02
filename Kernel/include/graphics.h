#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <fpmath.h>
#include <stdint.h>
#include <vga.h>

/*
 * Light types
 */
typedef enum {
  GFX_LIGHT_DIRECTIONAL,
  GFX_LIGHT_POINT,
} gfx_light_t;

/*
 * Light settings
 */
typedef enum {
  GFX_LIGHT_POSITION,
  GFX_LIGHT_COLOR,
  GFX_AMBIENT_LIGHT,
} gfx_lightSetting_t;

/*
 * MVP matrices
 */
typedef enum {
  GFX_MAT_MODEL,
  GFX_MAT_VIEW,
  GFX_MAT_PROJECTION,
} gfx_matrix_t;

/*
 * Render resolution settings
 */
typedef enum {
  GFX_RES_FULL,
  GFX_RES_HALF,
} gfx_res_t;

/*
 * Clear the internal frame and depth buffers.
 */
void gfx_clear(color_t color);

/*
 * Draw an array of primitives (triangles).
 * Takes the number of primitives. Vertex and normal arrays should have 3 * n
 * items. Set normal array to NULL for flat shading (fullbright).
 */
void gfx_drawPrimitives(
  float3 *vertices, float3 *normals, uint64_t n, float3 color
);

/*
 * Draw an array of primitives (triangles), using indexed arrays.
 * Takes the number of primitives. Index arrays should have 3 * n items.
 * Set normal array to NULL for flat shading (fullbright).
 */
void gfx_drawPrimitivesIndexed(
  float3 *vertices, float3 *normals, uint32_t *indices, uint32_t *normalIndices,
  uint64_t n, float3 color
);

/*
 * Draw an array of primitives as a wireframe.
 * Takes the number of primitives. Vertex array should have 3 * n items.
 */
void gfx_drawWireframe(float3 *vertices, uint64_t n, float3 color);

/*
 * Draw an array of primitives as a wireframe, using indexed arrays.
 * Takes the number of primitives. Index array should have 3 * n items.
 */
void gfx_drawWireframeIndexed(
  float3 *vertices, uint32_t *indices, uint64_t n, float3 color
);

/*
 * Set light properties.
 * The GFX_LIGHT_POSITION property is used as the (reverse) light direction
 * if the light type is set to GFX_LIGHT_DIRECTIONAL.
 */
void gfx_setLight(gfx_lightSetting_t which, float3 *data);

/*
 * Set the light type.
 * The graphics driver supports a single light at a time, that can be either
 * a directional or point light.
 */
void gfx_setLightType(gfx_light_t mode);

/*
 * Set one of the MVP matrices.
 */
void gfx_setMatrix(gfx_matrix_t which, float4x4 *data);

/*
 * Set render resolution.
 * The driver supports rendering at half resolution for increased performance.
 */
void gfx_setRenderResolution(gfx_res_t res);

/*
 * Present the internal framebuffer to the VGA driver's main framebuffer.
 * This function *does not* present to the screen, so that UI and other elements
 * may be drawn on top first. User must still call vga_present() to end a frame.
 */
void gfx_present();

/*
 * Parse a model in Wavefront OBJ format. Supports only the small subset of
 * features used by the renderer.
 * The arrays passed as parameters are assumed to have enough space. Passing a
 * smaller array than needed results in undefined behavior.
 */
void gfx_parseObj(
  const char *data, float3 *v, float3 *n, uint32_t *vi, uint32_t *ni
);

#endif
