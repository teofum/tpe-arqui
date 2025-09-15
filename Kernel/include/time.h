#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>

#define TICKS_PER_SECOND 1000

#define SECONDS 0x00
#define MINUTES 0x02
#define HOURS 0x04
#define DAY_OF_THE_WEEK 0x06
#define DAY_OF_THE_MONTH 0x07
#define MONTH 0x08
#define YEAR 0x09

typedef struct {
  int hours;
  int minutes;
  int seconds;
} time_t;

typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t day_of_week;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} datetime_t;

/*
 * Initialize the system timer to tick every 1ms
 */
void timer_init();

void timer_handler();

unsigned int ticks_elapsed();

unsigned int seconds_elapsed();

unsigned int minutes_elapsed();

unsigned int hours_elapsed();

uint8_t rtc_get_time(int descriptor);

datetime_t rtc_get_datetime();

datetime_t rtc_get_local_time(void);

#endif
