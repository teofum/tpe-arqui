#include <gfxdemo.h>
#include <kbd.h>
#include <print.h>
#include <syscall.h>
#include <vga.h>

uint8_t *image = (uint8_t *) 0x3000000;

int gfxdemo() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  uint64_t ticksElapsed = 0, ticksStart = _syscall(SYS_TICKS), frames = 0;
  uint8_t green = 0;
  int d = 1;
  int key = 0;
  while (!key) {
    vga_clear(0x00000080 | (green << 8));

    vga_bitmap(256, 128, image, 2, VGA_ALPHA_BLEND);

    green += d;
    if (green == 0xff || green == 0) d = -d;

    vga_rect(100, 100, 300, 300, 0xc0c0c0, 0);
    vga_shade(100, 100, 300, 300, 0xff0000, 0);
    vga_frame(100, 100, 300, 300, 0x80ff80, 0);

    for (uint32_t y = 100; y <= 300; y += 25) {
      vga_line(400, y, 600, y, 0xffffff, 0);
    }
    for (uint32_t x = 400; x <= 600; x += 25) {
      vga_line(x, 100, x, 300, 0xffffff, 0);
    }

    for (uint32_t y = 400; y <= 600; y += 25) {
      for (uint32_t x = 100; x <= 300; x += 25) {
        vga_line(200, 500, x, y, 0x00ffff, 0);
      }
    }

    vga_gradient(400, 400, 600, 500, colors(0xaa5540, 0xc0aa80), VGA_GRAD_H);
    vga_gradient(400, 500, 600, 600, colors(0xaa5540, 0xc0aa80), VGA_GRAD_V);

    vga_text(700, 100, "This is some text\nin multiple lines!", 0xffffff, 0, 0);
    vga_text(
      700, 132, "This\ttext\thas\ttab\tstops\n|\t|\t|\t|\t|", 0xffffff, 0, 0
    );

    // const vga_font_t *font = vga_font(vga_fontTiny);
    // vga_text(700, 174, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontTinyBold);
    // vga_text(700, 174 + 8, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontSmall);
    // vga_text(700, 178 + 16, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontDefault);
    // vga_text(700, 178 + 28, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontLarge);
    // vga_text(700, 178 + 44, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontAlt);
    // vga_text(700, 182 + 68, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontAltBold);
    // vga_text(700, 182 + 84, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontFuture);
    // vga_text(700, 186 + 100, "Hello world!", 0xffffff, 0, 0);
    // vga_font(vga_fontOld);
    // vga_text(700, 186 + 110, "Hello world!", 0xffffff, 0, 0);
    // vga_font(font);

    const char *longtext =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat.";

    vga_textWrap(700, 400, 200, longtext, colors(0xffffff, 0), VGA_WRAP_WORD);

    vga_rect(700, 400, 800, 600, 0x80ff80ff, VGA_ALPHA_BLEND);

    vga_text(420, 700, " Press any key to exit ", 0, 0xffffff, VGA_TEXT_INV);

    // Draw the frametime counter
    uint64_t frametimeMicros = frames == 0 ? 0 : ticksElapsed * 55000 / frames;
    uint64_t frametime = frametimeMicros / 1000;
    uint64_t fpsTimes100 =
      frametimeMicros == 0 ? 0 : 100000000 / frametimeMicros;
    uint64_t fps = fpsTimes100 / 100;
    frametimeMicros %= 1000;
    fpsTimes100 %= 100;

    char buf[50];
    sprintf(
      buf, "Frametime: %llu.%03llums (%llu.%02llu fps)", frametime,
      frametimeMicros, fps, fpsTimes100
    );
    vga_text(0, 0, buf, 0xffffff, 0, VGA_TEXT_BG);

    vga_present();

    key = kbd_getKeyEvent().key;
    frames++;
    ticksElapsed = _syscall(SYS_TICKS) - ticksStart;
  }

  // Restore status bar enabled state
  _syscall(SYS_STATUS_SET_ENABLED, statusEnabled);

  return 0;
}
