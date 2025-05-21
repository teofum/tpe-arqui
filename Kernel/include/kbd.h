#ifndef KBD_H
#define KBD_H
#include <stdint.h>

#define KBD_BUFFER_SIZE 128

typedef struct {
  uint8_t data[KBD_BUFFER_SIZE];
  int writePos, readPos;
} kbd_buffer_t;

typedef struct {
  uint8_t scancode;
  uint8_t isReleased;

  uint8_t shift : 1;
  uint8_t shift_r : 1;
  uint8_t backspace : 1;
  uint8_t ctrl : 1;
  uint8_t alt : 1;
  uint8_t caplock : 1;
} kbd_event_t;


/*
 * Called by keyboard interrupt handler.
 * Adds a scancode to the buffer, discarding oldest events fi we run out of
 * space.
 */
void kbd_addKeyEvent(uint8_t scancode);

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

/*
 * tiene que retornar un evento
 */
kbd_event_t kbd_getKeyEvent();

#endif
