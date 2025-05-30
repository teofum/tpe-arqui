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
  int32_t yi0 = ((v0.y + 1.0f) / 2.0f) * VGA_HEIGHT;
  int32_t yi1 = ((v1.y + 1.0f) / 2.0f) * VGA_HEIGHT;
  int32_t yi2 = ((v2.y + 1.0f) / 2.0f) * VGA_HEIGHT;

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
  uint32_t step = OFFSET_X;
  uint32_t line_offset = step * (right - left + 1);
  for (int32_t y = top; y <= bottom; y++) {
    for (int32_t x = left; x <= right; x++) {
      float xp = (x * 2.0f) / VGA_WIDTH - 1.0f;
      float yp = (y * 2.0f) / VGA_HEIGHT - 1.0f;

      float w0 = edgeFunction(v1.x, v1.y, v2.x, v2.y, xp, yp);
      float w1 = edgeFunction(v2.x, v2.y, v0.x, v0.y, xp, yp);
      float w2 = edgeFunction(v0.x, v0.y, v1.x, v1.y, xp, yp);

      if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
        w0 /= area;
        w1 /= area;
        w2 /= area;

        float z = v0.z * w0 + v1.z * w1 + v2.z * w2;

        float r = c0.x * w0 + c1.x * w1 + c2.x * w2;
        float g = c0.y * w0 + c1.y * w1 + c2.y * w2;
        float b = c0.z * w0 + c1.z * w1 + c2.z * w2;

        color_t color = rgba(
          (int) (r * 255.0f), (int) (g * 255.0f), (int) (b * 255.0f), 0xff
        );

        putpixel(fb, offset, color);
      }

      offset += step;
    }
    offset += OFFSET_Y - line_offset;
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
}

void gfx_drawPrimitives(float3 *vertices, uint64_t n, float3 color) {
  float4x4 persp = mat_perspective(M_PI * 0.5f, 0.75f, 0.1f, 100.0f);
  for (uint64_t i = 0; i < n; i++) {
    float4 v0 = vext(vertices[0], 1.0f);
    float4 v1 = vext(vertices[1], 1.0f);
    float4 v2 = vext(vertices[2], 1.0f);

    v0 = mvmul(persp, v0);
    v1 = mvmul(persp, v1);
    v2 = mvmul(persp, v2);

    drawTriangle(vpersp(v0), vpersp(v1), vpersp(v2), color, color, color);
    vertices += 3;
  }
}

/*
 * Copy the internal framebuffer to some other fb
 */
void gfx_present() { vga_copy(NULL, _gfxFramebuffer, 0); }
