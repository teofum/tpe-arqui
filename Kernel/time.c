#include <time.h>


unsigned long ticks = 0;

void timer_handler() { ticks++; }

unsigned int ticks_elapsed() { return ticks; }

unsigned int seconds_elapsed() { return ticks / TICKS_PER_SECOND; }

/* minutes elapsed in tick timer */
unsigned int minutes_elapsed() {
  return seconds_elapsed() % 3600;
}//(ticks/TICKS_PER_SECOND)%3600

/* hours elapsed in tick timer */
unsigned int hours_elapsed() {
  return minutes_elapsed() / 60;
}//((ticks/TICKS_PER_SECOND)%3600)/60

/* get HMS from timer tick in one struct */
Time_t getTimeElapsed(unsigned int ticks) {
  Time_t t;
  unsigned int total_seconds = ticks / TICKS_PER_SECOND;

  t.hours = total_seconds / 3600;
  t.minutes = (total_seconds % 3600) / 60;
  t.seconds = total_seconds % 60;

  return t;
}

// RTC UTC ( para argentina -3 horas ) ///////////////////////////////


uint8_t get_format(uint8_t num) {
  int dec = num & 240;
  dec = dec >> 4;
  int units = num & 15;
  return dec * 10 + units;
}

/* Fetches actual UTC real time */
uint8_t rtc_getTime(int descriptor) {
  uint8_t toReturn = asm_rtc_GetTime(descriptor);
  return get_format(toReturn);
}

/* Fetches whole YDMHMS in one struct */
DateTime_t rtc_getDateTime() {
  DateTime_t dt;
  dt.seconds = rtc_getTime(SECONDS);
  dt.minutes = rtc_getTime(MINUTES);
  dt.hours = rtc_getTime(HOURS);
  dt.day = rtc_getTime(DAY_OF_THE_MONTH);
  dt.month = rtc_getTime(MONTH);
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

DateTime_t rtc_getLocalTime(void) {
  DateTime_t dt = rtc_getDateTime();// Get UTC time

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

/// utils //////////////////////////////////////////////////

/* Pasa de uint8 to string usar para imprimir el datetime */
void uint8_to_string(uint8_t value, char *buffer) {
  int i = 0;

  if (value == 0) {
    buffer[i++] = '0';
    buffer[i] = '\0';
    return;
  }

  // Convert digits in reverse
  while (value > 0) {
    buffer[i++] = (value % 10) + '0';
    value /= 10;
  }

  buffer[i] = '\0';

  // Reverse the string
  int start = 0, end = i - 1;
  while (start < end) {
    char tmp = buffer[start];
    buffer[start] = buffer[end];
    buffer[end] = tmp;
    start++;
    end--;
  }
}
/// Ejemplo de uso //////////////
// char longtext[4] = {0};
// uint8_to_string(rtc_getDateTime().minutes, longtext);

