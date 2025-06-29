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
  GFX_HALFRES = 0x01,
  GFX_DEPTH_TEST = 0x02,
  GFX_DEPTH_WRITE = 0x04,
} gfx_flags_t;

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
 * Set renderer flags.
 * The driver supports rendering at half resolution for increased performance.
 */
void gfx_setFlag(gfx_flags_t flag, uint8_t value);

/*
 * Present the internal framebuffer to the VGA driver's main framebuffer.
 * This function *does not* present to the screen, so that UI and other elements
 * may be drawn on top first. User must still call vga_present() to end a frame.
 */
void gfx_present();

/*
 * Load model data from a binary model object.
 * Returns primitive count and sets the pointers passed as parameters to the
 * corresponding arrays in data.
 */
uint32_t
gfx_loadModel(void *data, float3 **v, float3 **n, uint32_t **vi, uint32_t **ni);

/*
 * Set the frame and depthbuffers used for drawing.
 * Simlar to vga_setFramebuffer, this function lets the user draw to an auxiliary
 * framebuffer for advanced composition effects.
 */
void gfx_setBuffers(uint8_t *framebuffer, float *depthbuffer);

/*
 * Copy the contents of one framebuffer to another. Similar to vga_copy, but
 * accounting for half-res mode.
 */
void gfx_copy(uint8_t *dst, uint8_t *src);

/*
 * Copy the contents of one depthbuffer to another.
 */
void gfx_depthcopy(float *dst, float *src);

/*
 * Get a pointer to the internal framebuffer. Useful if we want to draw on the
 * framebuffer directly, for example to combine 2D and 3D graphics.
 */
uint8_t *gfx_getFramebuffer();

#endif
