#include <print.h>
#include <process.h>
#include <status.h>
#include <stddef.h>
#include <time.h>
#include <vga.h>

#define CLOCK_WIDTH (STATUS_PADDING_X * 2 + 37 * 8)

uint8_t _status_enabled = 0;

const char *weekdays[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                          "Thursday", "Friday", "Saturday"};

const char *months[] = {"January",   "February", "March",    "April",
                        "May",       "June",     "July",     "August",
                        "September", "October",  "November", "December"};

void status_draw_statusbar() {
  if (!_status_enabled) return;
  uint32_t current_fb = proc_set_framebuffer(FB_DEFAULT);

  char buf[64];
  int32_t len;
  datetime_t t = rtc_get_local_time();

  // Draw statusbar background and border
  vga_gradient(
    0, 0, VGA_WIDTH - 1, STATUS_HEIGHT - 1, colors(0x0020a0, 0x2040c0),
    VGA_GRAD_V
  );
  vga_line(
    0, STATUS_HEIGHT - 1, VGA_WIDTH - 1, STATUS_HEIGHT - 1, 0x80ffffff,
    VGA_ALPHA_BLEND
  );
  vga_line(
    VGA_WIDTH - CLOCK_WIDTH, 0, VGA_WIDTH - CLOCK_WIDTH, STATUS_HEIGHT - 2,
    0x80ffffff, VGA_ALPHA_BLEND
  );

  // Draw system clock text
  vga_font_t oldfont = vga_font(VGA_FONT_ALT);
  uint8_t char_width = vga_getfont(VGA_FONT_ALT)->char_width;

  len = sprintf(
    buf, "%s, %s %02u 20%02u %02u:%02u:%02u", weekdays[t.day_of_week],
    months[t.month], t.day, t.year, t.hours, t.minutes, t.seconds
  );
  vga_text(
    VGA_WIDTH - len * char_width - STATUS_PADDING_X - 1, STATUS_PADDING_Y, buf,
    0xffffff, 0, 0
  );

  vga_font(oldfont);
  vga_present();

  proc_set_framebuffer(current_fb);
}

uint8_t status_enabled() { return _status_enabled; }

void status_set_enabled(uint8_t enabled) {
  _status_enabled = enabled;

  // Draw the statusbar immediately on enable
  if (_status_enabled) status_draw_statusbar();
}
