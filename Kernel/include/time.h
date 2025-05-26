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
} Time_t;

typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} DateTime_t;


void timer_handler();

unsigned int ticks_elapsed();

unsigned int seconds_elapsed();

unsigned int minutes_elapsed();

unsigned int hours_elapsed();

Time_t getTimeElapsed(unsigned int ticks);


extern uint8_t asm_rtc_GetTime(uint64_t descriptor);// de rtc.asm

uint8_t rtc_getTime(int descriptor);

DateTime_t rtc_getDateTime();


#endif
