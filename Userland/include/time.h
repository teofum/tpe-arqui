#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>

#define TICKS_PER_SECOND 1000

/*
 * Get the number of ticks since system start.
 * 1 tick = 1ms
 */
unsigned int time();

#endif
