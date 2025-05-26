#ifndef _TIME_H_
#define _TIME_H_
#include <stdint.h>

#define TICKS_PER_SECOND 18

#define SECONDS 00
#define MINUTES 02
#define HOURS 04
#define DAY_OF_THE_WEEK 06
#define DAY_OF_THE_MONTH 07
#define MONTH 08
#define YEAR 09

typedef struct {
  int hours;
  int minutes;
  int seconds;
} Time;

void timer_handler();

unsigned int ticks_elapsed();

unsigned int seconds_elapsed();

unsigned int minutes_elapsed();

unsigned int hours_elapsed();

Time getTimeElapsed(unsigned int ticks);


extern uint8_t asm_rtc_GetTime(uint64_t descriptor);// de rtc.asm

uint8_t rtc_getTime(int descriptor);

#endif
