#include <fpmath.h>
#include <graphics.h>
#include <mem.h>
#include <print.h>
#include <stddef.h>
#include <stdint.h>
#include <vga.h>

#define OFFSET_X (ctx->vbe_info.bpp >> 3)
#define OFFSET_Y (ctx->vbe_info.pitch)

#define pixel_offset(x, y) ((x) * OFFSET_X + (y) * OFFSET_Y)

#define b(c) ((c) & 0xff)
#define g(c) (((c) >> 8) & 0xff)
#define r(c) (((c) >> 16) & 0xff)

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define putpixel(fb, offset, color)                                            \
  fb->data[offset] = b(color), fb->data[offset + 1] = g(color),                \
  fb->data[offset + 2] = r(color)

struct gfx_framebuffer_cdt_t {
  uint32_t width;
  uint32_t height;
  uint8_t data[];
};

struct gfx_depthbuffer_cdt_t {
  uint32_t width;
  uint32_t height;
  float data[];
};

struct gfx_context_cdt_t {
  vbe_info_t vbe_info;

  gfx_framebuffer_t default_framebuffer;
  gfx_depthbuffer_t default_depthbuffer;
  gfx_framebuffer_t framebuffer;
  gfx_depthbuffer_t depthbuffer;

  uint32_t render_width;
  uint32_t render_height;

  float4x4 model;
  float4x4 normal_model;
  float4x4 view;
  float4x4 projection;
  float4x4 view_projection;

  float3 light_pos;
  float3 light_color;
  float3 ambient_light;
  gfx_light_t light_type;

  gfx_flags_t flags;
};

/*
 * Create a graphics context.
 * A usual frame looks something like this:
 *   - Clear frame and depth buffers
 *   - Update view and projection matrices, if necessary
 *   - Update lighting, if necessary
 *   - Call drawPrimitives to request a draw (potentially multiple times)
 */
gfx_context_t gfx_create_context() {
  gfx_context_t ctx = mem_alloc(sizeof(struct gfx_context_cdt_t));
  if (!ctx) return NULL;

  ctx->vbe_info = vga_get_vbe_info();

  ctx->framebuffer = gfx_create_framebuffer(ctx, VGA_AUTO, VGA_AUTO);
  ctx->depthbuffer = gfx_create_depthbuffer(ctx, VGA_AUTO, VGA_AUTO);
  ctx->default_framebuffer = ctx->framebuffer;
  ctx->default_depthbuffer = ctx->depthbuffer;

  ctx->render_width = ctx->framebuffer->width;
  ctx->render_height = ctx->framebuffer->height;

  ctx->model = mat_scale(1, 1, 1);
  ctx->normal_model = mat_scale(1, 1, 1);
  ctx->view = mat_scale(1, 1, 1);
  ctx->projection = mat_scale(1, 1, 1);
  ctx->view_projection = mat_scale(1, 1, 1);

  ctx->light_pos = (float3) {0, 0, 0};
  ctx->light_color = (float3) {0, 0, 0};
  ctx->ambient_light = (float3) {0, 0, 0};
  ctx->light_type = GFX_LIGHT_POINT;

  ctx->flags = GFX_DEPTH_TEST | GFX_DEPTH_WRITE;

  return ctx;
}

static inline float
edge_fn(float ax, float ay, float bx, float by, float px, float py) {
  return (by - ay) * (px - ax) - (bx - ax) * (py - ay);
}

static void draw_triangle(
  gfx_context_t ctx, float3 v0, float3 v1, float3 v2, float3 c0, float3 c1,
  float3 c2
) {
  int32_t xi0 = ((v0.x + 1.0f) / 2.0f) * ctx->render_width;
  int32_t xi1 = ((v1.x + 1.0f) / 2.0f) * ctx->render_width;
  int32_t xi2 = ((v2.x + 1.0f) / 2.0f) * ctx->render_width;
  int32_t yi0 =
    ctx->render_height - 1 - ((v0.y + 1.0f) / 2.0f) * ctx->render_height;
  int32_t yi1 =
    ctx->render_height - 1 - ((v1.y + 1.0f) / 2.0f) * ctx->render_height;
  int32_t yi2 =
    ctx->render_height - 1 - ((v2.y + 1.0f) / 2.0f) * ctx->render_height;

  // Calculate triangle bounds in screen space
  // Add one pixel padding to compensate for rounding errors
  int32_t top = min(yi0, min(yi1, yi2)) - 1;
  int32_t left = min(xi0, min(xi1, xi2)) - 1;
  int32_t bottom = max(yi0, max(yi1, yi2)) + 1;
  int32_t right = max(xi0, max(xi1, xi2)) + 1;

  // Intersect bounds with screen edges
  top = max(top, 0);
  left = max(left, 0);
  bottom = min(bottom, ctx->render_height - 1);
  right = min(right, ctx->render_width - 1);

  float inv_area = 1.0f / edge_fn(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);

  uint32_t offset = pixel_offset(left, top);
  uint32_t depth_offset = top * ctx->framebuffer->width + left;

  uint32_t step = OFFSET_X;
  uint32_t line_offset = step * (right - left + 1);
  uint32_t line_depth_offset = (right - left + 1);

  float xstep = 2.0f / ctx->render_width;
  float ystep = -2.0f / ctx->render_height;
  float xp = -1.0f + (left + 0.5f) * xstep;
  float yp = 1.0f + (top + 0.5f) * ystep;

  float w0s = edge_fn(v1.x, v1.y, v2.x, v2.y, xp, yp);
  float w1s = edge_fn(v2.x, v2.y, v0.x, v0.y, xp, yp);
  float w2s = edge_fn(v0.x, v0.y, v1.x, v1.y, xp, yp);
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
        float u = w0 * inv_area;
        float v = w1 * inv_area;
        float t = w2 * inv_area;

        drawn = 1;

        float z = v0.z * u + v1.z * v + v2.z * t;
        if (z >= 0.0f && z <= 1.0f &&
            (!(ctx->flags & GFX_DEPTH_TEST) ||
             (z + 0.0001f) < ctx->depthbuffer->data[depth_offset])) {
          if (ctx->flags & GFX_DEPTH_WRITE)
            ctx->depthbuffer->data[depth_offset] = z;

          float r = c0.x * u + c1.x * v + c2.x * t;
          float g = c0.y * u + c1.y * v + c2.y * t;
          float b = c0.z * u + c1.z * v + c2.z * t;

          color_t color = rgba(
            (int) (r * 255.0f), (int) (g * 255.0f), (int) (b * 255.0f), 0xff
          );

          putpixel(ctx->framebuffer, offset, color);
        }
      } else if (drawn) {
        // Triangles are convex shapes; if we've drawn a pixel on this line
        // and the next one is out of bounds, we know for sure all the pixels
        // to the right of it are also out of bounds.
        uint32_t rem = right - x + 1;
        offset += step * rem;
        depth_offset += rem;
        break;
      }

      offset += step;
      depth_offset += 1;

      w0 += w0xstep;
      w1 += w1xstep;
      w2 += w2xstep;
    }
    offset += OFFSET_Y - line_offset;
    depth_offset += ctx->framebuffer->width - line_depth_offset;

    w0s += w0ystep;
    w0 = w0s;
    w1s += w1ystep;
    w1 = w1s;
    w2s += w2ystep;
    w2 = w2s;
  }
}

void gfx_clear(gfx_context_t ctx, color_t color) {
  uint64_t *fb = (uint64_t *) ctx->framebuffer->data;
  uint64_t size = (OFFSET_Y >> 3) * ctx->framebuffer->height;

  if (ctx->vbe_info.bpp == 24) {
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
  } else {
    uint64_t data = color | ((uint64_t) color) << 32;
    for (uint64_t offset = 0; offset < size; offset++) { fb[offset] = data; }
  }

  uint64_t depthSize = ctx->depthbuffer->width * ctx->depthbuffer->height;
  for (uint64_t offset = 0; offset < depthSize; offset++) {
    ctx->depthbuffer->data[offset] = 999.0f;
  }
}

static void
draw_primitive_flat(gfx_context_t ctx, float3 *v, uint32_t *i, float3 color) {
  // Get vertex positions and transform to world space
  float4 v0 = vext(v[i[0]], 1.0f);
  float4 v1 = vext(v[i[1]], 1.0f);
  float4 v2 = vext(v[i[2]], 1.0f);

  v0 = mvmul(ctx->model, v0);
  v1 = mvmul(ctx->model, v1);
  v2 = mvmul(ctx->model, v2);

  // Transform to clip space
  v0 = mvmul(ctx->view_projection, v0);
  v1 = mvmul(ctx->view_projection, v1);
  v2 = mvmul(ctx->view_projection, v2);

  draw_triangle(ctx, vpersp(v0), vpersp(v1), vpersp(v2), color, color, color);
}

static inline void draw_primitive(
  gfx_context_t ctx, float3 *v, float3 *n, uint32_t *vi, uint32_t *ni,
  float3 color
) {
  float3 c[3];
  float3 vClip[3];

  for (uint32_t i = 0; i < 3; i++) {
    // Get vertex positions and transform to world space
    float4 vertex = vext(v[vi[i]], 1.0f);
    vertex = mvmul(ctx->model, vertex);

    // Perform per-vertex lighting calculations in world space
    float3 normal = n[ni[i]];
    normal = mvmul3(ctx->normal_model, normal);
    normal = vnorm(normal);

    float3 light;
    float intensity = 1.0f;
    switch (ctx->light_type) {
      case GFX_LIGHT_DIRECTIONAL:
        light = vnorm(ctx->light_pos);
        break;
      case GFX_LIGHT_POINT:
        light = vsub(ctx->light_pos, vred(vertex));
        float distanceSq = vabssq(light);
        intensity /= distanceSq;

        light = vnorm(light);
        break;
    }

    intensity *= vdot(normal, light);
    intensity = max(intensity, 0.0f);

    float3 light_color = vmuls(ctx->light_color, intensity);
    light_color = vadd(light_color, ctx->ambient_light);
    c[i] = vmul(color, light_color);
    c[i] = vsat(c[i]);

    // Transform vertex to clip space
    vertex = mvmul(ctx->view_projection, vertex);
    vClip[i] = vpersp(vertex);
  }

  draw_triangle(ctx, vClip[0], vClip[1], vClip[2], c[0], c[1], c[2]);
}

void gfx_draw_primitives(
  gfx_context_t ctx, float3 *vertices, float3 *normals, uint64_t n, float3 color
) {
  static uint32_t indices[] = {0, 1, 2};

  if (normals == NULL) {
    for (uint64_t i = 0; i < n; i++) {
      draw_primitive_flat(ctx, vertices, indices, color);
      vertices += 3;
    }
  } else {
    for (uint64_t i = 0; i < n; i++) {
      draw_primitive(ctx, vertices, normals, indices, indices, color);
      vertices += 3;
      normals += 3;
    }
  }
}

void gfx_draw_primitives_indexed(
  gfx_context_t ctx, float3 *vertices, float3 *normals, uint32_t *indices,
  uint32_t *normal_indices, uint64_t n, float3 color
) {
  if (normals == NULL) {
    for (uint64_t i = 0; i < n; i++) {
      draw_primitive_flat(ctx, vertices, indices, color);
      indices += 3;
    }
  } else {
    for (uint64_t i = 0; i < n; i++) {
      draw_primitive(ctx, vertices, normals, indices, normal_indices, color);
      indices += 3;
      normal_indices += 3;
    }
  }
}

static inline void
draw_wireframe_impl(gfx_context_t ctx, float3 *v, uint32_t *i, color_t color) {
  // Get vertex positions and transform to world space
  float4 v0 = vext(v[i[0]], 1.0f);
  float4 v1 = vext(v[i[1]], 1.0f);
  float4 v2 = vext(v[i[2]], 1.0f);

  v0 = mvmul(ctx->model, v0);
  v1 = mvmul(ctx->model, v1);
  v2 = mvmul(ctx->model, v2);

  // Transform to clip space
  v0 = mvmul(ctx->view_projection, v0);
  v1 = mvmul(ctx->view_projection, v1);
  v2 = mvmul(ctx->view_projection, v2);

  float3 v0f3 = vpersp(v0);
  float3 v1f3 = vpersp(v1);
  float3 v2f3 = vpersp(v2);

  // Convert to integer screen space coordinates
  int32_t xi0 = ((v0f3.x + 1.0f) / 2.0f) * ctx->framebuffer->width;
  int32_t xi1 = ((v1f3.x + 1.0f) / 2.0f) * ctx->framebuffer->width;
  int32_t xi2 = ((v2f3.x + 1.0f) / 2.0f) * ctx->framebuffer->width;
  int32_t yi0 = ctx->framebuffer->height - 1 -
                ((v0f3.y + 1.0f) / 2.0f) * ctx->framebuffer->height;
  int32_t yi1 = ctx->framebuffer->height - 1 -
                ((v1f3.y + 1.0f) / 2.0f) * ctx->framebuffer->height;
  int32_t yi2 = ctx->framebuffer->height - 1 -
                ((v2f3.y + 1.0f) / 2.0f) * ctx->framebuffer->height;

  xi0 = min(ctx->framebuffer->width - 1, max(0, xi0));
  xi1 = min(ctx->framebuffer->width - 1, max(0, xi1));
  xi2 = min(ctx->framebuffer->width - 1, max(0, xi2));
  yi0 = min(ctx->framebuffer->height - 1, max(0, yi0));
  yi1 = min(ctx->framebuffer->height - 1, max(0, yi1));
  yi2 = min(ctx->framebuffer->height - 1, max(0, yi2));

  // Draw lines
  vga_line(xi0, yi0, xi1, yi1, color, 0);
  vga_line(xi1, yi1, xi2, yi2, color, 0);
  vga_line(xi2, yi2, xi0, yi0, color, 0);
}

void gfx_draw_wireframe(
  gfx_context_t ctx, float3 *vertices, uint64_t n, float3 c
) {
  static uint32_t indices[] = {0, 1, 2};
  color_t color = rgba(
    (int) (c.x * 255.0f), (int) (c.y * 255.0f), (int) (c.z * 255.0f), 0xff
  );

  for (uint64_t i = 0; i < n; i++) {
    draw_wireframe_impl(ctx, vertices, indices, color);
    vertices += 3;
  }
}

void gfx_draw_wireframe_indexed(
  gfx_context_t ctx, float3 *vertices, uint32_t *indices, uint64_t n, float3 c
) {
  color_t color = rgba(
    (int) (c.x * 255.0f), (int) (c.y * 255.0f), (int) (c.z * 255.0f), 0xff
  );

  for (uint64_t i = 0; i < n; i++) {
    draw_wireframe_impl(ctx, vertices, indices, color);
    indices += 3;
  }
}

void gfx_set_light(gfx_context_t ctx, gfx_light_setting_t which, float3 *data) {
  switch (which) {
    case GFX_LIGHT_POSITION:
      ctx->light_pos = *data;
      break;
    case GFX_LIGHT_COLOR:
      ctx->light_color = *data;
      break;
    case GFX_AMBIENT_LIGHT:
      ctx->ambient_light = *data;
      break;
  }
}

void gfx_set_light_type(gfx_context_t ctx, gfx_light_t mode) {
  ctx->light_type = mode;
}

void gfx_set_matrix(gfx_context_t ctx, gfx_matrix_t which, float4x4 *data) {
  switch (which) {
    case GFX_MAT_MODEL:
      ctx->model = *data;
      ctx->normal_model = mtadj(ctx->model);
      break;
    case GFX_MAT_VIEW:
      ctx->view = *data;
      ctx->view_projection = mmul(ctx->projection, ctx->view);
      break;
    case GFX_MAT_PROJECTION:
      ctx->projection = *data;
      ctx->view_projection = mmul(ctx->projection, ctx->view);
      break;
  }
}

static void set_render_resolution(gfx_context_t ctx, int half) {
  ctx->render_width = ctx->framebuffer->width >> half;
  ctx->render_height = ctx->framebuffer->height >> half;
}

void gfx_set_flag(gfx_context_t ctx, gfx_flags_t flag, uint8_t value) {
  if (value) {
    ctx->flags |= flag;
  } else {
    ctx->flags &= ~flag;
  }

  if (flag == GFX_HALFRES) { set_render_resolution(ctx, value); }
}

/*
 * Copy the internal framebuffer to some other fb
 */
void gfx_present(gfx_context_t ctx) {
  if (ctx->flags & GFX_HALFRES) {
    vga_copy_ex(
      NULL, (vga_framebuffer_t) ctx->framebuffer,
      (vga_copy_ex_opts_t) {0, 0, 0, 0, ctx->framebuffer->width / 2,
                            ctx->framebuffer->height / 2, 2}
    );
  } else {
    vga_copy(NULL, (vga_framebuffer_t) ctx->framebuffer, 0);
  }
}

uint32_t gfx_load_model(
  const void *data, float3 **v, float3 **n, uint32_t **vi, uint32_t **ni
) {
  uint32_t *header = (uint32_t *) data;
  uint32_t n_verts = *header++;
  uint32_t n_normals = *header++;
  uint32_t n_faces = *header++;

  uint32_t v_offset = 0;
  uint32_t n_offset = v_offset + n_verts * 3;
  uint32_t vi_offset = n_offset + n_normals * 3;
  uint32_t ni_offset = vi_offset + n_faces * 3;

  *v = (float3 *) (header + v_offset);
  *n = (float3 *) (header + n_offset);
  *vi = header + vi_offset;
  *ni = header + ni_offset;

  return n_faces;
}

void gfx_set_buffers(
  gfx_context_t ctx, gfx_framebuffer_t framebuffer,
  gfx_depthbuffer_t depthbuffer
) {
  ctx->framebuffer = framebuffer == NULL ? ctx->framebuffer : framebuffer;
  ctx->depthbuffer = depthbuffer == NULL ? ctx->depthbuffer : depthbuffer;
}

static void memcpy64(uint64_t *dst, uint64_t *src, uint64_t len) {
  for (uint64_t i = 0; i < len; i++) { *dst++ = *src++; }
}

void gfx_copy(gfx_context_t ctx, gfx_framebuffer_t dst, gfx_framebuffer_t src) {
  if (dst == NULL) dst = ctx->default_framebuffer;
  if (src == NULL) src = ctx->default_framebuffer;

  size_t copy_width = min(dst->width, src->width);
  size_t copy_height = min(dst->height, src->height);
  size_t size = (copy_width * copy_height) / 2;

  memcpy64((uint64_t *) dst->data, (uint64_t *) src->data, size);
}

void gfx_depthcopy(
  gfx_context_t ctx, gfx_depthbuffer_t dst, gfx_depthbuffer_t src
) {
  if (dst == NULL) dst = ctx->default_depthbuffer;
  if (src == NULL) src = ctx->default_depthbuffer;

  size_t copy_width = min(dst->width, src->width);
  size_t copy_height = min(dst->height, src->height);
  size_t size = (copy_width * copy_height) / 2;

  memcpy64((uint64_t *) dst->data, (uint64_t *) src->data, size);
}

gfx_framebuffer_t
gfx_create_framebuffer(gfx_context_t ctx, int32_t width, int32_t height) {
  if (width <= 0) width += ctx->vbe_info.width;
  if (height <= 0) height += ctx->vbe_info.height;

  gfx_framebuffer_t fb = mem_alloc(width * height * sizeof(uint32_t) + 8);
  fb->width = width;
  fb->height = height;

  return fb;
}


gfx_depthbuffer_t
gfx_create_depthbuffer(gfx_context_t ctx, int32_t width, int32_t height) {
  if (width <= 0) width += ctx->vbe_info.width;
  if (height <= 0) height += ctx->vbe_info.height;

  gfx_depthbuffer_t db = mem_alloc(width * height * sizeof(float) + 8);
  db->width = width;
  db->height = height;

  return db;
}
