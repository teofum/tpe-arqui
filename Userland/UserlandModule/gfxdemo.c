#include <gfxdemo.h>
#include <syscall.h>

int gfxdemo() {
  sys_vga_clear(0x00000080);

  sys_vga_rect(100, 100, 300, 300, 0xc0c0c0, 0);
  sys_vga_shade(100, 100, 300, 300, 0xff0000, 0);
  sys_vga_frame(100, 100, 300, 300, 0x80ff80, 0);

  for (uint32_t y = 100; y <= 300; y += 25) {
    sys_vga_line(400, y, 600, y, 0xffffff, 0);
  }
  for (uint32_t x = 400; x <= 600; x += 25) {
    sys_vga_line(x, 100, x, 300, 0xffffff, 0);
  }

  for (uint32_t y = 400; y <= 600; y += 25) {
    for (uint32_t x = 100; x <= 300; x += 25) {
      sys_vga_line(200, 500, x, y, 0x00ffff, 0);
    }
  }

  sys_vga_gradient(
    400, 400, 600, 500, (0xaa5540ull << 32) | 0xc0aa80, VGA_GRAD_H
  );
  sys_vga_gradient(
    400, 500, 600, 600, (0xaa5540ull << 32) | 0xc0aa80, VGA_GRAD_V
  );

  sys_vga_text(
    700, 100, "This is some text\nin multiple lines!", 0xffffff, 0, 0
  );
  sys_vga_text(
    700, 132, "This\ttext\thas\ttab\tstops\n|\t|\t|\t|\t|", 0xffffff, 0, 0
  );

  // const vga_font_t *font = vga_font(vga_fontTiny);
  // sys_vga_text(700, 174, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontTinyBold);
  // sys_vga_text(700, 174 + 8, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontSmall);
  // sys_vga_text(700, 178 + 16, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontDefault);
  // sys_vga_text(700, 178 + 28, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontLarge);
  // sys_vga_text(700, 178 + 44, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontAlt);
  // sys_vga_text(700, 182 + 68, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontAltBold);
  // sys_vga_text(700, 182 + 84, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontFuture);
  // sys_vga_text(700, 186 + 100, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(vga_fontOld);
  // sys_vga_text(700, 186 + 110, "Hello world!", 0xffffff, 0, 0);
  // sys_vga_font(font);

  const char *longtext =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat.";

  sys_vga_textWrap(
    700, 400, 200, longtext, (0xffffffull << 32) | 0, VGA_WRAP_WORD
  );

  sys_vga_rect(700, 400, 800, 600, 0x80ff80ff, VGA_ALPHA_BLEND);

  sys_vga_text(420, 700, " Press any key to exit ", 0, 0xffffff, VGA_TEXT_INV);

  sys_vga_present();

  int read = 0;
  char buf[1];
  while (!read) { read = _syscall(SYS_READ, buf, 1); }

  return 0;
}
