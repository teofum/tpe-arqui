#include "fpmath.h"
#include "vga.h"
#include <graphics.h>
#include <stddef.h>

#define OFFSET_X (3)
#define OFFSET_Y (3 * VGA_WIDTH)
#define pixelOffset(x, y) ((x) * OFFSET_X + (y) * OFFSET_Y)

#define b(c) ((c) & 0xff)
#define g(c) (((c) >> 8) & 0xff)
#define r(c) (((c) >> 16) & 0xff)

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define putpixel(fb, offset, color)                                            \
  fb[offset] = b(color), fb[offset + 1] = g(color), fb[offset + 2] = r(color)

/*
 * Framebuffer for the graphics subsystem
 */
uint8_t _gfxFramebuffer[VGA_WIDTH * VGA_HEIGHT * 3];

/*
 * Depth buffer for 3d graphics functions.
 */
float _depthbuffer[VGA_WIDTH * VGA_HEIGHT];

/*
 * Graphics system state.
 * The graphics system is managed through system calls, modifying its internal
 * state to control rendering and requesting draws.
 * A usual frame looks something like this:
 *   - Clear frame and depth buffers
 *   - Update view and projection matrices, if necessary
 *   - Update lighting, if necessary
 *   - Call drawPrimitives to request a draw (potentially multiple times)
 */

float4x4 gfx_model;
float4x4 gfx_normalModel;
float4x4 gfx_view;
float4x4 gfx_projection;
float4x4 gfx_viewProjection;

float3 gfx_lightPos;
float3 gfx_lightColor;
float3 gfx_ambientLight;
gfx_light_t gfx_lightType = GFX_LIGHT_DIRECTIONAL;

/*
 * End of state.
 * Graphics functions below.
 */

static inline float
edgeFunction(float ax, float ay, float bx, float by, float px, float py) {
  return (by - ay) * (px - ax) - (bx - ax) * (py - ay);
}

/*
 * Draw a triangle
 */
static void
drawTriangle(float3 v0, float3 v1, float3 v2, float3 c0, float3 c1, float3 c2) {
  uint8_t *fb = _gfxFramebuffer;

  int32_t xi0 = ((v0.x + 1.0f) / 2.0f) * VGA_WIDTH;
  int32_t xi1 = ((v1.x + 1.0f) / 2.0f) * VGA_WIDTH;
  int32_t xi2 = ((v2.x + 1.0f) / 2.0f) * VGA_WIDTH;
  int32_t yi0 = VGA_HEIGHT - 1 - ((v0.y + 1.0f) / 2.0f) * VGA_HEIGHT;
  int32_t yi1 = VGA_HEIGHT - 1 - ((v1.y + 1.0f) / 2.0f) * VGA_HEIGHT;
  int32_t yi2 = VGA_HEIGHT - 1 - ((v2.y + 1.0f) / 2.0f) * VGA_HEIGHT;

  // Calculate triangle bounds in screen space
  int32_t top = min(yi0, min(yi1, yi2));
  int32_t left = min(xi0, min(xi1, xi2));
  int32_t bottom = max(yi0, max(yi1, yi2));
  int32_t right = max(xi0, max(xi1, xi2));

  // Intersect bounds with screen edges
  top = max(top, 0);
  left = max(left, 0);
  bottom = min(bottom, VGA_HEIGHT - 1);
  right = min(right, VGA_WIDTH - 1);

  float area = edgeFunction(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);

  uint32_t offset = pixelOffset(left, top);
  uint32_t depthOffset = top * VGA_WIDTH + left;

  uint32_t step = OFFSET_X;
  uint32_t line_offset = step * (right - left + 1);
  uint32_t line_depthOffset = (right - left + 1);
  for (int32_t y = top; y <= bottom; y++) {
    for (int32_t x = left; x <= right; x++) {
      float xp = (x * 2.0f + 1.0f) / VGA_WIDTH - 1.0f;
      float yp = (y * -2.0f - 1.0f) / VGA_HEIGHT + 1.0f;

      float w0 = edgeFunction(v1.x, v1.y, v2.x, v2.y, xp, yp);
      float w1 = edgeFunction(v2.x, v2.y, v0.x, v0.y, xp, yp);
      float w2 = edgeFunction(v0.x, v0.y, v1.x, v1.y, xp, yp);

      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        w0 /= area;
        w1 /= area;
        w2 /= area;

        float z = v0.z * w0 + v1.z * w1 + v2.z * w2;
        if (z >= 0.0f && z <= 1.0f &&
            (z + 0.0001f) < _depthbuffer[depthOffset]) {
          _depthbuffer[depthOffset] = z;

          float r = c0.x * w0 + c1.x * w1 + c2.x * w2;
          float g = c0.y * w0 + c1.y * w1 + c2.y * w2;
          float b = c0.z * w0 + c1.z * w1 + c2.z * w2;

          color_t color = rgba(
            (int) (r * 255.0f), (int) (g * 255.0f), (int) (b * 255.0f), 0xff
          );

          putpixel(fb, offset, color);
        }
      }

      offset += step;
      depthOffset += 1;
    }
    offset += OFFSET_Y - line_offset;
    depthOffset += VGA_WIDTH - line_depthOffset;
  }
}

void gfx_clear(color_t color) {
  uint64_t *fb = (uint64_t *) _gfxFramebuffer;
  uint64_t size = (OFFSET_Y >> 3) * VGA_HEIGHT;

  uint64_t c = color & 0xffffff;
  uint64_t data[] = {
    (c << 48) | (c << 24) | c,
    (c << 56) | (c << 32) | (c << 8) | (c >> 16),
    (c << 40) | (c << 16) | (c >> 8),
  };

  for (uint64_t offset = 0; offset < size; offset += 3) {
    fb[offset + 0] = data[0];
    fb[offset + 1] = data[1];
    fb[offset + 2] = data[2];
  }

  uint64_t depthSize = VGA_WIDTH * VGA_HEIGHT;
  for (uint64_t offset = 0; offset < depthSize; offset++) {
    _depthbuffer[offset] = 999.0f;
  }
}

static inline void drawPrimitiveFlat(float3 *v, uint32_t *i, float3 color) {
  // Get vertex positions and transform to world space
  float4 v0 = vext(v[i[0]], 1.0f);
  float4 v1 = vext(v[i[1]], 1.0f);
  float4 v2 = vext(v[i[2]], 1.0f);

  v0 = mvmul(gfx_model, v0);
  v1 = mvmul(gfx_model, v1);
  v2 = mvmul(gfx_model, v2);

  // Transform to clip space
  v0 = mvmul(gfx_viewProjection, v0);
  v1 = mvmul(gfx_viewProjection, v1);
  v2 = mvmul(gfx_viewProjection, v2);

  drawTriangle(vpersp(v0), vpersp(v1), vpersp(v2), color, color, color);
}

static inline void
drawPrimitive(float3 *v, float3 *n, uint32_t *vi, uint32_t *ni, float3 color) {
  float3 c[3];
  float3 vClip[3];

  for (uint32_t i = 0; i < 3; i++) {
    // Get vertex positions and transform to world space
    float4 vertex = vext(v[vi[i]], 1.0f);
    vertex = mvmul(gfx_model, vertex);

    // Perform per-vertex lighting calculations in world space
    float3 normal = n[ni[i]];
    normal = mvmul3(gfx_normalModel, normal);
    normal = vnorm(normal);

    float3 light;
    switch (gfx_lightType) {
      case GFX_LIGHT_DIRECTIONAL:
        light = vnorm(gfx_lightPos);
        break;
      case GFX_LIGHT_POINT:
        light = vnorm(vsub(gfx_lightPos, vred(vertex)));
    }

    float intensity = vdot(normal, light);
    intensity = max(intensity, 0.0f);

    float3 lightColor = vmuls(gfx_lightColor, intensity);
    lightColor = vadd(lightColor, gfx_ambientLight);
    c[i] = vmul(color, lightColor);

    // Transform to clip space
    vertex = mvmul(gfx_viewProjection, vertex);
    vClip[i] = vpersp(vertex);
  }

  drawTriangle(vClip[0], vClip[1], vClip[2], c[0], c[1], c[2]);
}

void gfx_drawPrimitives(
  float3 *vertices, float3 *normals, uint64_t n, float3 color
) {
  static uint32_t indices[] = {0, 1, 2};

  if (normals == NULL) {
    for (uint64_t i = 0; i < n; i++) {
      drawPrimitiveFlat(vertices, indices, color);
      vertices += 3;
    }
  } else {
    for (uint64_t i = 0; i < n; i++) {
      drawPrimitive(vertices, normals, indices, indices, color);
      vertices += 3;
      normals += 3;
    }
  }
}

void gfx_drawPrimitivesIndexed(
  float3 *vertices, float3 *normals, uint32_t *indices, uint32_t *normalIndices,
  uint64_t n, float3 color
) {
  if (normals == NULL) {
    for (uint64_t i = 0; i < n; i++) {
      drawPrimitiveFlat(vertices, indices, color);
      indices += 3;
    }
  } else {
    for (uint64_t i = 0; i < n; i++) {
      drawPrimitive(vertices, normals, indices, normalIndices, color);
      indices += 3;
      normalIndices += 3;
    }
  }
}

void gfx_setLight(gfx_lightSetting_t which, float3 *data) {
  switch (which) {
    case GFX_LIGHT_POSITION:
      gfx_lightPos = *data;
      break;
    case GFX_LIGHT_COLOR:
      gfx_lightColor = *data;
      break;
    case GFX_AMBIENT_LIGHT:
      gfx_ambientLight = *data;
      break;
  }
}

void gfx_setLightType(gfx_light_t mode) { gfx_lightType = mode; }

void gfx_setMatrix(gfx_matrix_t which, float4x4 *data) {
  switch (which) {
    case GFX_MAT_MODEL:
      gfx_model = *data;
      gfx_normalModel = mtadj(gfx_model);
      break;
    case GFX_MAT_VIEW:
      gfx_view = *data;
      gfx_viewProjection = mmul(gfx_projection, gfx_view);
      break;
    case GFX_MAT_PROJECTION:
      gfx_projection = *data;
      gfx_viewProjection = mmul(gfx_projection, gfx_view);
      break;
  }
}

/*
 * Copy the internal framebuffer to some other fb
 */
void gfx_present() { vga_copy(NULL, _gfxFramebuffer, 0); }
