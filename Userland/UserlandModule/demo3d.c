#include <gfxdemo.h>
#include <graphics.h>
#include <kbd.h>
#include <print.h>
#include <syscall.h>
#include <vga.h>

#define deg2rad(x) ((x) / 180.0f * M_PI)

int demo3d() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  uint64_t ticksElapsed = 0, ticksStart = _syscall(SYS_TICKS), frames = 0;
  float angle = 0;

  /*
   * Set up graphics system
   */
  float3 pos = {0, 0, 4};
  float3 target = {0, 0, 0};
  float3 up = {0, 1, 0};

  float4x4 view = mat_lookat(pos, target, up);
  float4x4 projection = mat_perspective(1.5f, 4.0f / 3.0f, 0.1f, 10.0f);

  gfx_setMatrix(GFX_MAT_VIEW, &view);
  gfx_setMatrix(GFX_MAT_PROJECTION, &projection);

  float3 light = {-1, 1, 0.5};
  float3 lightcolor = {1, 1, 1};
  float3 ambient = {0.1, 0.1, 0.1};

  gfx_setLightType(GFX_LIGHT_DIRECTIONAL);
  gfx_setLight(GFX_LIGHT_POSITION, &light);
  gfx_setLight(GFX_AMBIENT_LIGHT, &ambient);
  gfx_setLight(GFX_LIGHT_COLOR, &lightcolor);

  float3 colors[] = {
    {0.0, 0.0, 0.5}, {0.0, 0.5, 0.0}, {0.5, 0.0, 0.0},
    {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0},
  };
  float3 v[] = {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1, 1},
                {1, -1, -1},  {1, -1, 1},  {1, 1, -1},  {1, 1, 1}};
  float3 n[] = {
    {0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0},
  };
  uint32_t vi[] = {0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 4, 1, 5, 4,
                   2, 6, 3, 6, 7, 3, 0, 4, 2, 4, 6, 2, 3, 5, 1, 3, 7, 5};
  uint32_t ni[] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3,
                   2, 2, 2, 2, 2, 2, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4};

  int key = 0;
  while (!key) {
    gfx_clear(0x200020);

    float4x4 model = mat_rotationY(angle);
    gfx_setMatrix(GFX_MAT_MODEL, &model);

    gfx_drawPrimitivesIndexed(v, n, vi, ni, 12, colors[0]);

    model = mmul(model, mat_scale(0.7f, 0.7f, 0.7f));
    model = mmul(mat_translation(1, 1, 1), model);
    gfx_setMatrix(GFX_MAT_MODEL, &model);

    gfx_drawPrimitivesIndexed(v, n, vi, ni, 12, colors[1]);
    gfx_present();

    uint64_t frametime = frames == 0 ? 0 : ticksElapsed * 55 / frames;
    char buf[50];
    sprintf(buf, "Frametime: %llums", frametime);
    vga_text(0, 0, buf, 0xffffff, 0, VGA_TEXT_BG);

    vga_present();

    angle += 0.01;
    if (angle > M_PI) angle -= 2.0f * M_PI;

    key = kbd_getKeyEvent().key;
    frames++;
    ticksElapsed = _syscall(SYS_TICKS) - ticksStart;
  }

  // Restore status bar enabled state
  _syscall(SYS_STATUS_SET_ENABLED, statusEnabled);

  return 0;
}
