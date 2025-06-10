#include <fpmath.h>
#include <graphics.h>
#include <print.h>
#include <stddef.h>
#include <stdint.h>
#include <vga.h>

#define OFFSET_X (VBE_mode_info->bpp >> 3)
#define OFFSET_Y (VBE_mode_info->pitch)
#define pixelOffset(x, y) ((x) * OFFSET_X + (y) * OFFSET_Y)

#define b(c) ((c) & 0xff)
#define g(c) (((c) >> 8) & 0xff)
#define r(c) (((c) >> 16) & 0xff)

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define putpixel(fb, offset, color)                                            \
  fb[offset] = b(color), fb[offset + 1] = g(color), fb[offset + 2] = r(color)

/*
 * Default buffers
 */
static uint8_t gfx_defaultFramebuffer[FRAMEBUFFER_SIZE];
static float gfx_defaultDepthbuffer[VGA_MAX_WIDTH * VGA_MAX_HEIGHT];

/*
 * Framebuffer for the graphics subsystem
 */
static uint8_t *_gfxFramebuffer = gfx_defaultFramebuffer;

/*
 * Depth buffer for 3d graphics functions.
 */
static float *_depthbuffer = gfx_defaultDepthbuffer;

/*
 * Render resolution settings.
 * Rendering at half resolution is supported for faster rendering.
 */
static uint32_t gfx_renderWidth;
static uint32_t gfx_renderHeight;

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

static float4x4 gfx_model;
static float4x4 gfx_normalModel;
static float4x4 gfx_view;
static float4x4 gfx_projection;
static float4x4 gfx_viewProjection;

static float3 gfx_lightPos;
static float3 gfx_lightColor;
static float3 gfx_ambientLight;
static gfx_light_t gfx_lightType = GFX_LIGHT_DIRECTIONAL;

static gfx_flags_t gfx_flags = GFX_DEPTH_TEST | GFX_DEPTH_WRITE;

void gfx_init() {
  gfx_renderWidth = VGA_WIDTH;
  gfx_renderHeight = VGA_HEIGHT;
}

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
  int32_t xi0 = ((v0.x + 1.0f) / 2.0f) * gfx_renderWidth;
  int32_t xi1 = ((v1.x + 1.0f) / 2.0f) * gfx_renderWidth;
  int32_t xi2 = ((v2.x + 1.0f) / 2.0f) * gfx_renderWidth;
  int32_t yi0 =
    gfx_renderHeight - 1 - ((v0.y + 1.0f) / 2.0f) * gfx_renderHeight;
  int32_t yi1 =
    gfx_renderHeight - 1 - ((v1.y + 1.0f) / 2.0f) * gfx_renderHeight;
  int32_t yi2 =
    gfx_renderHeight - 1 - ((v2.y + 1.0f) / 2.0f) * gfx_renderHeight;

  // Calculate triangle bounds in screen space
  // Add one pixel padding to compensate for rounding errors
  int32_t top = min(yi0, min(yi1, yi2)) - 1;
  int32_t left = min(xi0, min(xi1, xi2)) - 1;
  int32_t bottom = max(yi0, max(yi1, yi2)) + 1;
  int32_t right = max(xi0, max(xi1, xi2)) + 1;

  // Intersect bounds with screen edges
  top = max(top, 0);
  left = max(left, 0);
  bottom = min(bottom, gfx_renderHeight - 1);
  right = min(right, gfx_renderWidth - 1);

  float invArea = 1.0f / edgeFunction(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);

  uint32_t offset = pixelOffset(left, top);
  uint32_t depthOffset = top * VGA_WIDTH + left;

  uint32_t step = OFFSET_X;
  uint32_t line_offset = step * (right - left + 1);
  uint32_t line_depthOffset = (right - left + 1);

  float xstep = 2.0f / gfx_renderWidth;
  float ystep = -2.0f / gfx_renderHeight;
  float xp = -1.0f + (left + 0.5f) * xstep;
  float yp = 1.0f + (top + 0.5f) * ystep;

  float w0s = edgeFunction(v1.x, v1.y, v2.x, v2.y, xp, yp);
  float w1s = edgeFunction(v2.x, v2.y, v0.x, v0.y, xp, yp);
  float w2s = edgeFunction(v0.x, v0.y, v1.x, v1.y, xp, yp);
  float w0 = w0s, w1 = w1s, w2 = w2s;

  float w0xstep = (v2.y - v1.y) * xstep;
  float w1xstep = (v0.y - v2.y) * xstep;
  float w2xstep = (v1.y - v0.y) * xstep;
  float w0ystep = (v1.x - v2.x) * ystep;
  float w1ystep = (v2.x - v0.x) * ystep;
  float w2ystep = (v0.x - v1.x) * ystep;

  // Early exit optimization variable
  uint32_t drawn;

  for (int32_t y = top; y <= bottom; y++) {
    drawn = 0;
    for (int32_t x = left; x <= right; x++) {
      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        float u = w0 * invArea;
        float v = w1 * invArea;
        float t = w2 * invArea;

        drawn = 1;

        float z = v0.z * u + v1.z * v + v2.z * t;
        if (z >= 0.0f && z <= 1.0f &&
            (!(gfx_flags & GFX_DEPTH_TEST) ||
             (z + 0.0001f) < _depthbuffer[depthOffset])) {
          if (gfx_flags & GFX_DEPTH_WRITE) _depthbuffer[depthOffset] = z;

          float r = c0.x * u + c1.x * v + c2.x * t;
          float g = c0.y * u + c1.y * v + c2.y * t;
          float b = c0.z * u + c1.z * v + c2.z * t;

          color_t color = rgba(
            (int) (r * 255.0f), (int) (g * 255.0f), (int) (b * 255.0f), 0xff
          );

          putpixel(_gfxFramebuffer, offset, color);
        }
      } else if (drawn) {
        // Triangles are convex shapes; if we've drawn a pixel on this line
        // and the next one is out of bounds, we know for sure all the pixels
        // to the right of it are also out of bounds.
        uint32_t rem = right - x + 1;
        offset += step * rem;
        depthOffset += rem;
        break;
      }

      offset += step;
      depthOffset += 1;

      w0 += w0xstep;
      w1 += w1xstep;
      w2 += w2xstep;
    }
    offset += OFFSET_Y - line_offset;
    depthOffset += VGA_WIDTH - line_depthOffset;

    w0s += w0ystep;
    w0 = w0s;
    w1s += w1ystep;
    w1 = w1s;
    w2s += w2ystep;
    w2 = w2s;
  }
}

void gfx_clear(color_t color) {
  uint64_t *fb = (uint64_t *) _gfxFramebuffer;
  uint64_t size = (OFFSET_Y >> 3) * gfx_renderHeight;

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

  uint64_t depthSize = VGA_WIDTH * gfx_renderHeight;
  for (uint64_t offset = 0; offset < depthSize; offset++) {
    _depthbuffer[offset] = 999.0f;
  }
}

static void drawPrimitiveFlat(float3 *v, uint32_t *i, float3 color) {
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
    float intensity = 1.0f;
    switch (gfx_lightType) {
      case GFX_LIGHT_DIRECTIONAL:
        light = vnorm(gfx_lightPos);
        break;
      case GFX_LIGHT_POINT:
        light = vsub(gfx_lightPos, vred(vertex));
        float distanceSq = vabssq(light);
        intensity /= distanceSq;

        light = vnorm(light);
        break;
    }

    intensity *= vdot(normal, light);
    intensity = max(intensity, 0.0f);

    float3 lightColor = vmuls(gfx_lightColor, intensity);
    lightColor = vadd(lightColor, gfx_ambientLight);
    c[i] = vmul(color, lightColor);
    c[i] = vsat(c[i]);

    // Transform vertex to clip space
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

static inline void drawWireframeImpl(float3 *v, uint32_t *i, color_t color) {
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

  float3 v0f3 = vpersp(v0);
  float3 v1f3 = vpersp(v1);
  float3 v2f3 = vpersp(v2);

  // Convert to integer screen space coordinates
  int32_t xi0 = ((v0f3.x + 1.0f) / 2.0f) * gfx_renderWidth;
  int32_t xi1 = ((v1f3.x + 1.0f) / 2.0f) * gfx_renderWidth;
  int32_t xi2 = ((v2f3.x + 1.0f) / 2.0f) * gfx_renderWidth;
  int32_t yi0 =
    gfx_renderHeight - 1 - ((v0f3.y + 1.0f) / 2.0f) * gfx_renderHeight;
  int32_t yi1 =
    gfx_renderHeight - 1 - ((v1f3.y + 1.0f) / 2.0f) * gfx_renderHeight;
  int32_t yi2 =
    gfx_renderHeight - 1 - ((v2f3.y + 1.0f) / 2.0f) * gfx_renderHeight;

  xi0 = min(VGA_WIDTH - 1, max(0, xi0));
  xi1 = min(VGA_WIDTH - 1, max(0, xi1));
  xi2 = min(VGA_WIDTH - 1, max(0, xi2));
  yi0 = min(VGA_HEIGHT - 1, max(0, yi0));
  yi1 = min(VGA_HEIGHT - 1, max(0, yi1));
  yi2 = min(VGA_HEIGHT - 1, max(0, yi2));

  // Draw lines
  vga_line(xi0, yi0, xi1, yi1, color, 0);
  vga_line(xi1, yi1, xi2, yi2, color, 0);
  vga_line(xi2, yi2, xi0, yi0, color, 0);
}

void gfx_drawWireframe(float3 *vertices, uint64_t n, float3 c) {
  static uint32_t indices[] = {0, 1, 2};
  color_t color = rgba(
    (int) (c.x * 255.0f), (int) (c.y * 255.0f), (int) (c.z * 255.0f), 0xff
  );

  vga_setFramebuffer(_gfxFramebuffer);
  for (uint64_t i = 0; i < n; i++) {
    drawWireframeImpl(vertices, indices, color);
    vertices += 3;
  }
  vga_setFramebuffer(NULL);
}

void gfx_drawWireframeIndexed(
  float3 *vertices, uint32_t *indices, uint64_t n, float3 c
) {
  color_t color = rgba(
    (int) (c.x * 255.0f), (int) (c.y * 255.0f), (int) (c.z * 255.0f), 0xff
  );

  vga_setFramebuffer(_gfxFramebuffer);
  for (uint64_t i = 0; i < n; i++) {
    drawWireframeImpl(vertices, indices, color);
    indices += 3;
  }
  vga_setFramebuffer(NULL);
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

static void setRenderResolution(int half) {
  gfx_renderWidth = half ? (VGA_WIDTH >> 1) : VGA_WIDTH;
  gfx_renderHeight = half ? (VGA_HEIGHT >> 1) : VGA_HEIGHT;
}

void gfx_setFlag(gfx_flags_t flag, uint8_t value) {
  if (value) {
    gfx_flags |= flag;
  } else {
    gfx_flags &= ~flag;
  }

  if (flag == GFX_HALFRES) { setRenderResolution(value); }
}

/*
 * Copy the internal framebuffer to some other fb
 */
void gfx_present() {
  if (gfx_flags & GFX_HALFRES) {
    vga_copy2x(NULL, _gfxFramebuffer);
  } else {
    vga_copy(NULL, _gfxFramebuffer, 0);
  }
}

uint32_t gfx_loadModel(
  void *data, float3 **v, float3 **n, uint32_t **vi, uint32_t **ni
) {
  uint32_t *header = (uint32_t *) data;
  uint32_t nVerts = *header++;
  uint32_t nNormals = *header++;
  uint32_t nFaces = *header++;

  uint32_t vOffset = 0;
  uint32_t nOffset = vOffset + nVerts * 3;
  uint32_t viOffset = nOffset + nNormals * 3;
  uint32_t niOffset = viOffset + nFaces * 3;

  *v = (float3 *) (header + vOffset);
  *n = (float3 *) (header + nOffset);
  *vi = header + viOffset;
  *ni = header + niOffset;

  return nFaces;
}

void gfx_setBuffers(uint8_t *framebuffer, float *depthbuffer) {
  _gfxFramebuffer = framebuffer == NULL ? gfx_defaultFramebuffer : framebuffer;
  _depthbuffer = depthbuffer == NULL ? gfx_defaultDepthbuffer : depthbuffer;
}

static void memcpy64(uint64_t *dst, uint64_t *src, uint64_t len) {
  for (uint64_t i = 0; i < len; i++) { *dst++ = *src++; }
}

void gfx_copy(uint8_t *dst, uint8_t *src) {
  if (dst == NULL) dst = gfx_defaultFramebuffer;
  if (src == NULL) src = gfx_defaultFramebuffer;

  memcpy64(
    (uint64_t *) dst, (uint64_t *) src, (OFFSET_Y >> 3) * gfx_renderHeight
  );
}

void gfx_depthcopy(float *dst, float *src) {
  if (dst == NULL) dst = gfx_defaultDepthbuffer;
  if (src == NULL) src = gfx_defaultDepthbuffer;

  memcpy64(
    (uint64_t *) dst, (uint64_t *) src, (VGA_WIDTH >> 1) * gfx_renderHeight
  );
}

uint8_t *gfx_getFramebuffer() { return _gfxFramebuffer; }
