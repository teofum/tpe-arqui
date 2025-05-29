#include <print.h>
#include <time.h>
#include <vga.h>

#define STATUS_HEIGHT 24
#define STATUS_PADDING_X 8
#define STATUS_PADDING_Y 4

#define bcd_decode(x) ((((x) & 0xf0) >> 4) * 10 + ((x) & 0x0f))

extern uint8_t _rtc_getTime(uint64_t descriptor);// de rtc.asm

uint64_t ticks = 0;

const char *weekdays[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                          "Thursday", "Friday", "Saturday"};

const char *months[] = {"January",   "February", "March",    "April",
                        "May",       "June",     "July",     "August",
                        "September", "October",  "November", "December"};

/*
 * Draw a system-wide status bar, showing the system clock.
 * Can be used for other useful information in future.
 */
static void drawStatusBar() {
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

void timer_handler() {
  ticks++;
  if (!(ticks % (TICKS_PER_SECOND))) { drawStatusBar(); }
}

unsigned int ticks_elapsed() { return ticks; }

unsigned int seconds_elapsed() { return ticks / TICKS_PER_SECOND; }

/* minutes elapsed in tick timer */
unsigned int minutes_elapsed() { return (seconds_elapsed() / 60) % 60; }

/* hours elapsed in tick timer */
unsigned int hours_elapsed() { return (minutes_elapsed() / 60) % 24; }

/* get HMS from timer tick in one struct */
time_t getTimeElapsed(unsigned int ticks) {
  time_t t;
  unsigned int total_seconds = ticks / TICKS_PER_SECOND;

  t.hours = (total_seconds / 3600) % 24;
  t.minutes = (total_seconds % 3600) / 60;
  t.seconds = total_seconds % 60;

  return t;
}

/* Fetches actual UTC real time */
uint8_t rtc_getTime(int descriptor) {
  uint8_t toReturn = _rtc_getTime(descriptor);
  return bcd_decode(toReturn);
}

/* Fetches whole YDMHMS in one struct */
dateTime_t rtc_getDateTime() {
  dateTime_t dt;
  dt.seconds = rtc_getTime(SECONDS);
  dt.minutes = rtc_getTime(MINUTES);
  dt.hours = rtc_getTime(HOURS);
  dt.day = rtc_getTime(DAY_OF_THE_MONTH);
  dt.dayOfWeek = rtc_getTime(DAY_OF_THE_WEEK) - 1;
  dt.month = rtc_getTime(MONTH) - 1;
  dt.year = rtc_getTime(YEAR);
  return dt;
}

static uint8_t is_leap_year(uint8_t year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint8_t days_in_month(uint8_t month, uint8_t year) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};

  if (month == 2 && is_leap_year(year)) { return 29; }

  return days[month - 1];
}

dateTime_t rtc_getLocalTime(void) {
  dateTime_t dt = rtc_getDateTime();// Get UTC time

  // Argentina (UTC-3)
  if (dt.hours >= 3) {
    dt.hours -= 3;
  } else {
    dt.hours = 24 + dt.hours - 3;
    if (dt.day > 1) {
      dt.day--;
    } else {
      if (dt.month > 1) {
        dt.month--;
        dt.day = days_in_month(dt.month, dt.year);
      } else {
        dt.month = 12;
        dt.day = 31;
        dt.year--;
      }
    }
  }

  return dt;
}
