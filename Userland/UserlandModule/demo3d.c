#include <gfxdemo.h>
#include <graphics.h>
#include <kbd.h>
#include <print.h>
#include <syscall.h>
#include <vga.h>

#define deg2rad(x) ((x) / 180.0f * M_PI)

static void *obj_utah = (void *) 0x3007800;

float3 *demo3d_v;
float3 *demo3d_n;
uint32_t *demo3d_vi;
uint32_t *demo3d_ni;

int demo3d() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  uint64_t frametime = 0, ticksTotal = _syscall(SYS_TICKS);
  float angle = 0;
  int wireframe = 0;

  /*
   * Set up graphics system
   */

  // Set half render resolution for speed
  uint8_t halfres = 1;
  gfx_setFlag(GFX_HALFRES, halfres);

  // Set up view and projection matrices
  float3 pos = {0, 2, 4};
  float3 target = {0, 0, 0};
  float3 up = {0, 1, 0};

  float4x4 view = mat_lookat(pos, target, up);
  gfx_setMatrix(GFX_MAT_VIEW, &view);

  vbe_info_t info = vga_getVBEInfo();
  float aspect = (float) info.width / info.height;
  float fovDegrees = 75.0f;
  float4x4 projection =
    mat_perspective(deg2rad(fovDegrees), aspect, 0.1f, 10.0f);
  gfx_setMatrix(GFX_MAT_PROJECTION, &projection);

  // Light colors
  uint32_t lightColorIdx = 0;
  float3 lightColors[] = {
    {1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 1.0},
    {1.0, 0.0, 0.0}, {1.0, 0.0, 1.0}, {1.0, 1.0, 0.0},
  };

  // Set up lighting
  float3 light = {-1, 0.5, -0.5};
  float3 lightcolor = lightColors[lightColorIdx];
  float3 ambient = {0.1, 0.1, 0.1};

  gfx_light_t lightType = GFX_LIGHT_DIRECTIONAL;
  gfx_setLightType(lightType);
  gfx_setLight(GFX_LIGHT_POSITION, &light);
  gfx_setLight(GFX_AMBIENT_LIGHT, &ambient);
  gfx_setLight(GFX_LIGHT_COLOR, &lightcolor);

  // Material colors
  uint32_t colorIdx = 0;
  float3 colors[] = {
    {0.8, 0.8, 0.8}, {0.8, 0.4, 0.3}, {0.5, 0.4, 0.8}, {0.1, 0.8, 0.3},
    {0.8, 0.2, 0.6}, {0.7, 0.8, 0.3}, {0.1, 0.2, 0.4}, {0.3, 0.7, 0.7},
  };

  // Load the teapot model
  uint32_t primCount =
    gfx_loadModel(obj_utah, &demo3d_v, &demo3d_n, &demo3d_vi, &demo3d_ni);

  /*
   * Render loop
   */
  int exit = 0;
  while (!exit) {
    // Check input
    kbd_pollEvents();

    if (kbd_keypressed(KEY_ESCAPE) || kbd_keypressed(KEY_RETURN)) exit = 1;
    if (kbd_keypressed(KEY_C))
      colorIdx = (colorIdx + 1) % (sizeof(colors) / sizeof(float3));
    if (kbd_keypressed(KEY_L)) {
      lightColorIdx =
        (lightColorIdx + 1) % (sizeof(lightColors) / sizeof(float3));
      lightcolor = lightColors[lightColorIdx];
      gfx_setLight(GFX_LIGHT_COLOR, &lightcolor);
    }
    if (kbd_keypressed(KEY_K)) {
      if (lightType == GFX_LIGHT_DIRECTIONAL) {
        lightType = GFX_LIGHT_POINT;
        light.y = 0.5;
      } else {
        lightType = GFX_LIGHT_DIRECTIONAL;
        light.x = -1.0;
        light.y = 0.5;
        light.z = 0.5;
        gfx_setLight(GFX_LIGHT_POSITION, &light);
      }

      gfx_setLightType(lightType);
    }
    if (kbd_keypressed(KEY_W)) { wireframe = !wireframe; }
    if (kbd_keypressed(KEY_R)) {
      halfres = !halfres;
      gfx_setFlag(GFX_HALFRES, halfres);
    }
    if (kbd_keypressed(KEY_COMMA)) {
      fovDegrees += 5.0f;
      projection = mat_perspective(deg2rad(fovDegrees), aspect, 0.1f, 10.0f);
      gfx_setMatrix(GFX_MAT_PROJECTION, &projection);
    }
    if (kbd_keypressed(KEY_PERIOD)) {
      fovDegrees -= 5.0f;
      projection = mat_perspective(deg2rad(fovDegrees), aspect, 0.1f, 10.0f);
      gfx_setMatrix(GFX_MAT_PROJECTION, &projection);
    }
    if (kbd_keypressed(KEY_ARROW_UP)) {
      pos.y += 0.5f;
      view = mat_lookat(pos, target, up);
      gfx_setMatrix(GFX_MAT_VIEW, &view);
    }
    if (kbd_keypressed(KEY_ARROW_DOWN)) {
      pos.y -= 0.5f;
      view = mat_lookat(pos, target, up);
      gfx_setMatrix(GFX_MAT_VIEW, &view);
    }
    if (kbd_keypressed(KEY_ARROW_LEFT)) {
      pos.z += 0.5f;
      view = mat_lookat(pos, target, up);
      gfx_setMatrix(GFX_MAT_VIEW, &view);
    }
    if (kbd_keypressed(KEY_ARROW_RIGHT)) {
      pos.z -= 0.5f;
      view = mat_lookat(pos, target, up);
      gfx_setMatrix(GFX_MAT_VIEW, &view);
    }

    // For point light, make it orbit around the model
    if (lightType == GFX_LIGHT_POINT) {
      light.x = 2.5f * sin(angle);
      light.z = -2.5f * cos(angle);
      gfx_setLight(GFX_LIGHT_POSITION, &light);
    }

    // Clear the internal framebuffer and depth buffer
    gfx_clear(0x000000);

    // Create a model matrix and set it
    float4x4 model = mat_rotationY(angle);
    model = mmul(model, mat_translation(0, -1.0, 0));
    gfx_setMatrix(GFX_MAT_MODEL, &model);

    // Draw the triangles
    if (wireframe) {
      gfx_drawWireframeIndexed(
        demo3d_v, demo3d_vi, primCount, colors[colorIdx]
      );
    } else {
      gfx_drawPrimitivesIndexed(
        demo3d_v, demo3d_n, demo3d_vi, demo3d_ni, primCount, colors[colorIdx]
      );
    }

    // Present the framebuffer to the video driver main framebuffer
    gfx_present();

    // Draw the frametime counter
    uint64_t fpsTimes100 = frametime == 0 ? 0 : 100000 / frametime;
    uint64_t fps = fpsTimes100 / 100;
    fpsTimes100 %= 100;

    char buf[50];
    sprintf(
      buf, "Frametime: %llums (%llu.%02llu fps)", frametime, fps, fpsTimes100
    );
    vga_text(0, 0, buf, 0xffffff, 0, VGA_TEXT_BG);

    // Draw text hints
    vga_text(0, 32, "[C] Change model color", 0xffffff, 0, VGA_TEXT_BG);
    vga_text(0, 48, "[L] Change light color", 0xffffff, 0, VGA_TEXT_BG);
    vga_text(
      0, 64, "[K] Change light type (directional/point)", 0xffffff, 0,
      VGA_TEXT_BG
    );
    vga_text(0, 80, "[W] Toggle wireframe rendering", 0xffffff, 0, VGA_TEXT_BG);
    vga_text(0, 96, "[R] Toggle half resolution", 0xffffff, 0, VGA_TEXT_BG);
    vga_text(0, 112, "[,] [.] Change FOV", 0xffffff, 0, VGA_TEXT_BG);
    vga_text(
      0, 128, "[Up] [Down] Move camera up/down", 0xffffff, 0, VGA_TEXT_BG
    );
    vga_text(
      0, 144, "[Left] [Right] Move camera back/forward", 0xffffff, 0,
      VGA_TEXT_BG
    );
    vga_text(0, 160, "[Esc] or [Return] Exit", 0xffffff, 0, VGA_TEXT_BG);

    // Present the main framebuffer to the screen
    vga_present();

    // Update variables
    angle += 0.01;
    if (angle > M_PI) angle -= 2.0f * M_PI;

    uint64_t ticks = _syscall(SYS_TICKS);
    frametime = ticks - ticksTotal;
    ticksTotal = ticks;
  }

  // Restore status bar enabled state
  _syscall(SYS_STATUS_SET_ENABLED, statusEnabled);

  return 0;
}
