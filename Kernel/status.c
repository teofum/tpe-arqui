#include <print.h>
#include <status.h>
#include <time.h>
#include <vga.h>

#define CLOCK_WIDTH (STATUS_PADDING_X * 2 + 37 * 8)

uint8_t _statusEnabled = 1;

const char *weekdays[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                          "Thursday", "Friday", "Saturday"};

const char *months[] = {"January",   "February", "March",    "April",
                        "May",       "June",     "July",     "August",
                        "September", "October",  "November", "December"};

void drawStatusBar() {
  if (!_statusEnabled) return;

  // Get local time
  char buf[64];
  int32_t len;
  dateTime_t t = rtc_getLocalTime();

  // Draw statusbar background and border
  vga_gradient(
    0, 0, VGA_WIDTH - 1, STATUS_HEIGHT - 1, colors(0x0020a0, 0x2040c0),
    VGA_GRAD_V
  );
  vga_line(0, STATUS_HEIGHT - 1, VGA_WIDTH - 1, STATUS_HEIGHT - 1, 0xffffff, 0);
  vga_line(
    VGA_WIDTH - CLOCK_WIDTH, 0, VGA_WIDTH - CLOCK_WIDTH, STATUS_HEIGHT - 1,
    0xffffff, 0
  );

  // Draw system clock text
  const vga_font_t *oldfont = vga_font(vga_fontAlt);

  len = sprintf(
    buf, "%s, %s %02u 20%02u %02u:%02u:%02u", weekdays[t.dayOfWeek],
    months[t.month], t.day, t.year, t.hours, t.minutes, t.seconds
  );
  vga_text(
    VGA_WIDTH - len * vga_fontAlt->charWidth - STATUS_PADDING_X - 1,
    STATUS_PADDING_Y, buf, 0xffffff, 0, 0
  );

  vga_font(oldfont);
  vga_present();
}

uint8_t status_enabled() { return _statusEnabled; }

void status_setEnabled(uint8_t enabled) { _statusEnabled = enabled; }
