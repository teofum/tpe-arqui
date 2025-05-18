#ifndef KBD_H
#define KBD_H
#include <stdint.h>

#define KBD_BUFFER_SIZE 128

typedef struct {
  uint8_t data[KBD_BUFFER_SIZE];
  int writePos, readPos;
} kbd_buffer_t;

/*
 * Consumes all events (scancodes) in queue and updates keyboard state
 */
void kbd_pollEvents();

/*
 * Returns 1 if key is pressed, 0 if not.
 */
int kbd_keydown(uint8_t key);

/*
 * Returns 1 if key was pressed since last pollEvents call.
 */
int kbd_keypressed(uint8_t key);

/*
 * Returns 1 if key was pressed since last pollEvents call.
 */
int kbd_keyreleased(uint8_t key);

#endif
