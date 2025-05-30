#include <gfxdemo.h>
#include <graphics.h>
#include <kbd.h>
#include <print.h>
#include <syscall.h>
#include <vga.h>

int demo3d() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  uint64_t ticksElapsed = 0, ticksStart = _syscall(SYS_TICKS), frames = 0;
  float angle = 0;
  int key = 0;
  while (!key) {
    float3 v[] = {
      {0, 0, -1},
      {sin(angle), cos(angle), -1},
      {cos(angle), -sin(angle), -1},
      {0, 0, -2},
      {0, -1, -2},
      {-1, 0, -2}
    };
    float3 color = {0.5, 0.25, 0.75};

    gfx_clear(0x200020);
    gfx_drawPrimitives(v, 2, color);
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
