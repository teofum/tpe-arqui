#include <audio.h>
#include <print.h>
#include <status.h>
#include <time.h>
#include <vga.h>
#include <audio.h>

#define bcd_decode(x) ((((x) & 0xf0) >> 4) * 10 + ((x) & 0x0f))

extern uint8_t _rtc_getTime(uint64_t descriptor);// de rtc.asm
extern void audio_timer_tick(void);

static uint64_t timer_ticks = 0;

/*
 * Escribe un byte en un puerto de E/S
 */
extern void outb(uint16_t port, uint8_t value);

void timer_init() {
  // Set mode 3, channel 0, high/low byte
  outb(0x43, 0x36);

  uint16_t div = 1193;// 1193182 Hz / 1000 Hz = 1193
  outb(0x40, div & 0xff);
  outb(0x40, div >> 8);
}

void timer_handler() {
  timer_ticks++;
  if (!(timer_ticks % (TICKS_PER_SECOND))) { status_drawStatusBar(); }
  
  audio_timer_tick();
}

unsigned int ticks_elapsed() { return timer_ticks; }

unsigned int seconds_elapsed() { return timer_ticks / TICKS_PER_SECOND; }

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

