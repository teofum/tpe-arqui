#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

#define STATUS_HEIGHT 24
#define STATUS_PADDING_X 8
#define STATUS_PADDING_Y 4

/*
 * Return whether the status bar is enabled.
 */
uint8_t status_enabled();

/*
 * Enable or disable the status bar.
 * If enabled, the status bar is automatically drawn to the screen once every
 * second.
 */
void status_set_enabled(uint8_t enabled);

#endif
