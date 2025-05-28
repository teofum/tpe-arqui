#ifndef KBD_H
#define KBD_H
#include <stdint.h>

typedef struct {
  uint8_t key;
  uint8_t isReleased;

  uint8_t shift : 1;
  uint8_t shift_r : 1;
  uint8_t ctrl : 1;
  uint8_t alt : 1;
  uint8_t capslock : 1;
} kbd_event_t;

/*
 * Consumes all events (scancodes) in queue and updates keyboard state
 */
extern void kbd_pollEvents();

/*
 * Returns 1 if key is pressed, 0 if not.
 */
extern int kbd_keydown(uint8_t key);

/*
 * Returns 1 if key was pressed since last pollEvents call.
 */
extern int kbd_keypressed(uint8_t key);

/*
 * Returns 1 if key was pressed since last pollEvents call.
 */
extern int kbd_keyreleased(uint8_t key);

/*
 * Returns the next keyboard event.
 */
extern kbd_event_t kbd_getKeyEvent();

/*
 * Returns the ASCII character corresponding to the next keyboard event.
 * If there are no events in queue, returns -1.
 */
extern int kbd_getchar();

#endif
