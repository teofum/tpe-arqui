#ifndef _TIME_H_
#define _TIME_H_
#include <stdint.h>

#define TICKS_PER_SECOND 18

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
  uint8_t dayOfWeek;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} dateTime_t;


void timer_handler();

unsigned int ticks_elapsed();

unsigned int seconds_elapsed();

unsigned int minutes_elapsed();

unsigned int hours_elapsed();

time_t getTimeElapsed(unsigned int ticks);

uint8_t rtc_getTime(int descriptor);

dateTime_t rtc_getDateTime();

dateTime_t rtc_getLocalTime(void);

#endif
