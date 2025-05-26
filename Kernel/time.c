#include <time.h>


unsigned long ticks = 0;

void timer_handler() { ticks++; }

unsigned int ticks_elapsed() { return ticks; }

unsigned int seconds_elapsed() { return ticks / TICKS_PER_SECOND; }

unsigned int minutes_elapsed() {
  return seconds_elapsed() % 3600;
}//(ticks/TICKS_PER_SECOND)%3600

unsigned int hours_elapsed() {
  return minutes_elapsed() / 60;
}//((ticks/TICKS_PER_SECOND)%3600)/60

Time getTimeElapsed(unsigned int ticks) {
  Time t;
  unsigned int total_seconds = ticks / TICKS_PER_SECOND;

  t.hours = total_seconds / 3600;
  t.minutes = (total_seconds % 3600) / 60;
  t.seconds = total_seconds % 60;

  return t;
}

// RTC ///////////////////////////////

uint8_t get_format(uint8_t num) {//chequiar, lo saque de un tp q me pasaron
  int dec = num & 240;
  dec = dec >> 4;
  int units = num & 15;
  return dec * 10 + units;
}

uint8_t rtc_getTime(int descriptor) {
  uint8_t toReturn = asm_rtc_GetTime(descriptor);
  return get_format(toReturn);
}
