#include <stdint.h>
#include <string.h>

#include <kbd.h>
#include <string.h>

#define scancodeToKey(x) ((x) & 0x7f)
#define isRelease(x) ((x) & 0x80)
#define isSpecial(x)                                                           \
  (((x) == SC_CTRL) || ((x) == SC_LSHIFT) || ((x) == SC_RSHIFT) ||             \
   ((x) == SC_ALT) || ((x) == SC_CAPSLOCK))

#define next(x) x = (x + 1) % KBD_BUFFER_SIZE

typedef enum {
  SC_CTRL = 0x1D,// Left Ctrl
  SC_LSHIFT = 0x2A,
  SC_RSHIFT = 0x36,
  SC_ALT = 0x38,// Left Alt
  SC_CAPSLOCK = 0x3A,
} ScancodeSpecial;

kbd_buffer_t kbd_buffer = {
  .data = {0},
  .writePos = 0,
  .readPos = 0,
};

uint8_t kbd_state[128] = {0};
uint8_t kbd_lastState[128] = {0};
uint8_t kbd_capslock = 0;

int kbd_key2ascii[128] = {
  -1,  0x1B, '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
  '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
  '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
  ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
  '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   '7', '8', '9', '-', '4', '5', '6',
  '+', '1',  '2',  '3', '0',  '.', 0,   0,   0,   0,   0
};

int kbd_key2ascii_shift[128] = {
  -1,   0x1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  '\b',
  '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
  'A',  'S',  'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z',
  'X',  'C',  'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,    0,
  0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   '7', '8', '9',  '-',
  '4',  '5',  '6', '+', '1', '2', '3', '0', '.', 0,   0,   0,   0,   0
};

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

kbd_event_t kbd_getKeyEvent() {
  kbd_event_t kbd_event = {0};
  kbd_event.key = 0;

  while (kbd_buffer.readPos != kbd_buffer.writePos) {
    uint8_t scancode = kbd_buffer.data[kbd_buffer.readPos];
    next(kbd_buffer.readPos);

    uint8_t key = scancodeToKey(scancode);
    if ((isSpecial(scancode) != 0) || isRelease(scancode)) {
      if (scancode == SC_CAPSLOCK) {
        // Special handling for caps lock, it acts as a toggle
        if (!isRelease(scancode)) kbd_capslock = !kbd_capslock;
      }
      kbd_state[key] = isRelease(scancode) ? 0 : 1;
    } else {
      kbd_state[key] = 1;
      kbd_event.key = key;
      kbd_event.isReleased = isRelease(scancode);

      kbd_event.alt = kbd_state[SC_ALT];
      kbd_event.ctrl = kbd_state[SC_CTRL];
      kbd_event.shift = kbd_state[SC_LSHIFT];
      kbd_event.shift_r = kbd_state[SC_RSHIFT];
      kbd_event.capslock = kbd_capslock;

      return kbd_event;
    }
  }

  return kbd_event;
}

int kbd_getchar() {
  kbd_event_t ev = kbd_getKeyEvent();

  int isShifted = ev.shift || ev.shift_r || ev.capslock;
  return isShifted ? kbd_key2ascii_shift[ev.key] : kbd_key2ascii[ev.key];
}
