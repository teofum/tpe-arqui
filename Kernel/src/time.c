#include <audio.h>
#include <lib.h>
#include <print.h>
#include <process.h>
#include <scheduler.h>
#include <status.h>
#include <time.h>
#include <vga.h>

#define bcd_decode(x) ((((x) & 0xf0) >> 4) * 10 + ((x) & 0x0f))

extern uint8_t _rtc_get_time(uint64_t descriptor);// de rtc.asm
extern void audio_timer_tick(void);

static uint64_t timer_ticks = 0;

void timer_init() {
  // Set mode 3, channel 0, high/low byte
  outb(0x43, 0x36);

  uint16_t div = 1193;// 1193182 Hz / 1000 Hz = 1193
  outb(0x40, div & 0xff);
  outb(0x40, div >> 8);
}

void timer_handler() {
  timer_ticks++;
  // Update every eighth a second so we don't skip seconds every now and then
  if (!(timer_ticks % (TICKS_PER_SECOND >> 3))) { status_draw_statusbar(); }

  audio_timer_tick();

  // Run scheduler every 50 ticks (about 20hz)
  if (scheduler_force_next) {
    // kinda hacky
    scheduler_force_next = 0;
    scheduler_next();
  } else if (!(timer_ticks % 50)) {
    scheduler_enqueue(proc_running_pid);
    scheduler_next();
  }
}

unsigned int ticks_elapsed() { return timer_ticks; }

unsigned int seconds_elapsed() { return timer_ticks / TICKS_PER_SECOND; }

/* minutes elapsed in tick timer */
unsigned int minutes_elapsed() { return (seconds_elapsed() / 60) % 60; }

/* hours elapsed in tick timer */
unsigned int hours_elapsed() { return (minutes_elapsed() / 60) % 24; }

/* get HMS from timer tick in one struct */
static time_t get_time_elapsed(unsigned int ticks) {
  time_t t;
  unsigned int total_seconds = ticks / TICKS_PER_SECOND;

  t.hours = (total_seconds / 3600) % 24;
  t.minutes = (total_seconds % 3600) / 60;
  t.seconds = total_seconds % 60;

  return t;
}

/* Fetches actual UTC real time */
uint8_t rtc_get_time(int descriptor) {
  uint8_t rtc_time = _rtc_get_time(descriptor);
  return bcd_decode(rtc_time);
}

/* Fetches whole YDMHMS in one struct */
datetime_t rtc_get_datetime() {
  datetime_t dt;
  dt.seconds = rtc_get_time(SECONDS);
  dt.minutes = rtc_get_time(MINUTES);
  dt.hours = rtc_get_time(HOURS);
  dt.day = rtc_get_time(DAY_OF_THE_MONTH);
  dt.day_of_week = rtc_get_time(DAY_OF_THE_WEEK) - 1;
  dt.month = rtc_get_time(MONTH) - 1;
  dt.year = rtc_get_time(YEAR);
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

datetime_t rtc_get_local_time(void) {
  datetime_t dt = rtc_get_datetime();// Get UTC time

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
