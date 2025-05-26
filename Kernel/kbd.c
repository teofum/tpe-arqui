#include <stdint.h>
#include <string.h>

#include <kbd.h>
#include <string.h>

typedef enum {
  SC_CTRL = 0x1D,// Left Ctrl
  SC_LSHIFT = 0x2A,
  SC_RSHIFT = 0x36,
  SC_ALT = 0x38,// Left Alt
  SC_CAPLOCK = 0x3A,
} ScancodeSpecial;

#define scancodeToKey(x) ((x) & 0x7f)
#define isRelease(x) ((x) & 0x80)
#define isSpecial(x)                                                           \
  (((x) == SC_CTRL) || ((x) == SC_LSHIFT) || ((x) == SC_RSHIFT) ||             \
   ((x) == SC_ALT) || ((x) == SC_CAPLOCK))

#define next(x) x = (x + 1) % KBD_BUFFER_SIZE

kbd_buffer_t kbd_buffer = {
  .data = {0},
  .writePos = 0,
  .readPos = 0,
};

uint8_t kbd_state[128] = {0};
uint8_t kbd_lastState[128] = {0};

/*
 * Called by keyboard interrupt handler.
 * Adds a scancode to the buffer, discarding oldest events fi we run out of
 * space.
 */
void kbd_addKeyEvent(uint8_t sc) {
  kbd_buffer.data[kbd_buffer.writePos] = sc;
  next(kbd_buffer.writePos);

  // If we ran into the start of the queue, get rid of the older events
  if (kbd_buffer.writePos == kbd_buffer.readPos) next(kbd_buffer.readPos);
  return;
}

void kbd_pollEvents() {
  memcpy(kbd_lastState, kbd_state, sizeof(kbd_state));

  while (kbd_buffer.readPos != kbd_buffer.writePos) {
    uint8_t scancode = kbd_buffer.data[kbd_buffer.readPos];
    if (scancodeToKey(scancode) == SC_CAPLOCK) {//  togle caplock on press
      if (!(isRelease(scancode))) {
        kbd_state[scancodeToKey(scancode)] =
          !(kbd_state[scancodeToKey(scancode)]);
      }
    }
    kbd_state[scancodeToKey(scancode)] = isRelease(scancode) ? 0 : 1;
    next(kbd_buffer.readPos);
  }
}

int kbd_keydown(uint8_t key) { return kbd_state[key]; }

int kbd_keypressed(uint8_t key) {
  return (kbd_state[key] && !kbd_lastState[key]);
}

int kbd_keyreleased(uint8_t key) {
  return (!kbd_state[key] && kbd_lastState[key]);
}

/*
 * tiene que retornar un evento
 */
kbd_event_t kbd_getKeyEvent() {
  kbd_event_t kbd_event = {0};
  kbd_event.scancode = 0;

  while (kbd_buffer.readPos != kbd_buffer.writePos) {
    uint8_t scancode = kbd_buffer.data[kbd_buffer.readPos];
    next(kbd_buffer.readPos);

    if ((isSpecial(scancode) != 0) || isRelease(scancode)) {
      // es una especial o un release ? actualiza el estado del teclado:;

      kbd_state[scancodeToKey(scancode)] = isRelease(scancode) ? 0 : 1;
    } else {
      // es un press no especial ? lo actualiza en el estado y lo meta al
      // y lo meta al state:;

      kbd_state[scancodeToKey(scancode)] = 1;
      kbd_event.scancode = scancodeToKey(scancode);
      kbd_event.isReleased = isRelease(scancode);

      kbd_event.alt = kbd_state[SC_ALT];
      kbd_event.caplock = kbd_state[SC_CAPLOCK];
      kbd_event.ctrl = kbd_state[SC_CTRL];
      kbd_event.shift = kbd_state[SC_LSHIFT];
      kbd_event.shift_r = kbd_state[SC_RSHIFT];

      return kbd_event;
    }
  }

  return kbd_event;
}
