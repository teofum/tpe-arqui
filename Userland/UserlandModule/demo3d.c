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
  float3 pos = {0, 0, 5};
  float3 target = {0, 0, 0};
  float3 up = {0, 1, 0};

  float4x4 projection = mat_perspective(1.5f, 4.0f / 3.0f, 0.1f, 10.0f);

  gfx_setMatrix(GFX_MAT_PROJECTION, &projection);

  float3 colors[] = {
    {0.0, 0.0, 0.5}, {0.0, 0.5, 0.0}, {0.5, 0.0, 0.0},
    {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.5, 0.0},
  };
  float3 v[] = {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1, 1},
                {1, -1, -1},  {1, -1, 1},  {1, 1, -1},  {1, 1, 1}};
  uint32_t i[] = {0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 4, 1, 5, 4,
                  2, 6, 3, 6, 7, 3, 0, 4, 2, 4, 6, 2, 3, 5, 1, 3, 7, 5};

  int key = 0;
  while (!key) {
    gfx_clear(0x200020);

    pos.x = 4 * sin(angle);
    pos.y = 2 * sin(angle);
    pos.z = 4 * cos(angle);
    float4x4 view = mat_lookat(pos, target, up);
    gfx_setMatrix(GFX_MAT_VIEW, &view);

    gfx_drawPrimitivesIndexed(v, i, 12, colors[0]);

    view = mmul(view, mat_translation(0, 2, -1));
    gfx_setMatrix(GFX_MAT_VIEW, &view);

    gfx_drawPrimitivesIndexed(v, i, 12, colors[1]);
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
