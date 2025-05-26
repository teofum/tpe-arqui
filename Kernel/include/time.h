#ifndef _TIME_H_
#define _TIME_H_

#define TICKS_PER_SECOND 18

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

Time getTime(unsigned int ticks);

#endif
