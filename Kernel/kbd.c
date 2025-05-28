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

typedef enum {
  SC_EXT_MM_PREV = 0x10,
  SC_EXT_MM_NEXT = 0x19,
  SC_EXT_MM_MUTE = 0x20,
  SC_EXT_MM_CALC = 0x21,
  SC_EXT_MM_PLAY = 0x22,
  SC_EXT_MM_STOP = 0x24,
  SC_EXT_MM_VOL_DOWN = 0x2E,
  SC_EXT_MM_VOL_UP = 0x30,
  SC_EXT_MM_WWW_HOME = 0x32,
  SC_EXT_KEYPAD_ENTER = 0x1C,
  SC_EXT_KEYPAD_DIV = 0x35,
  SC_EXT_RIGHT_CTRL = 0x1D,
  SC_EXT_RIGHT_ALT = 0x38,
  SC_EXT_HOME = 0x47,
  SC_EXT_END = 0x4F,
  SC_EXT_PAGEUP = 0x49,
  SC_EXT_PAGEDOWN = 0x51,
  SC_EXT_INSERT = 0x52,
  SC_EXT_DELETE = 0x53,
  SC_EXT_ARROW_UP = 0x48,
  SC_EXT_ARROW_DOWN = 0x50,
  SC_EXT_ARROW_LEFT = 0x4B,
  SC_EXT_ARROW_RIGHT = 0x4D,
  SC_EXT_LEFT_GUI = 0x5B,
  SC_EXT_RIGHT_GUI = 0x5C,
  SC_EXT_APPS = 0x5D,
  SC_EXT_ACPI_POWER = 0x5E,
  SC_EXT_ACPI_SLEEP = 0x5F,
  SC_EXT_ACPI_WAKE = 0x63,
  SC_EXT_MM_WWW_SEARCH = 0x65,
  SC_EXT_MM_WWW_FAV,
  SC_EXT_MM_WWW_REFRESH,
  SC_EXT_MM_WWW_STOP,
  SC_EXT_MM_WWW_FORWARD,
  SC_EXT_MM_WWW_BACK,
  SC_EXT_MM_MY_COMPUTER,
  SC_EXT_MM_EMAIL,
  SC_EXT_MM_MEDIA_SELECT,
} ext_scancode_t;

kbd_buffer_t kbd_buffer = {
  .data = {0},
  .writePos = 0,
  .readPos = 0,
};

uint8_t kbd_state[128] = {0};
uint8_t kbd_lastState[128] = {0};
uint8_t kbd_capslock = 0;
uint8_t kbd_extended = 0;

int kbd_key2ascii[128] = {
  -1,  0x1B, '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
  '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
  '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
  ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
  '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   '7', '8', '9', '-', '4', '5', '6',
  '+', '1',  '2',  '3', '0',  '.', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   '\n', '/', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,
};

int kbd_key2ascii_shift[128] = {
  -1,  0x1B, '!',  '@', '#',  '$', '%', '^', '&', '*', '(', ')', '_',
  '+', '\b', '\t', 'Q', 'W',  'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
  '{', '}',  '\n', 0,   'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
  ':', '"',  '~',  0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
  '>', '?',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   '7', '8', '9', '-', '4', '5', '6',
  '+', '1',  '2',  '3', '0',  '.', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   '\n', '/', 0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,
};

static uint8_t getExtendedKey(uint8_t key) {
  // A table would look nicer, but the data is too sparse for it to be worth it
  switch (key) {
    case SC_EXT_MM_PREV:
      return KEY_MM_PREV;
    case SC_EXT_MM_NEXT:
      return KEY_MM_NEXT;
    case SC_EXT_MM_MUTE:
      return KEY_MM_MUTE;
    case SC_EXT_MM_CALC:
      return KEY_MM_CALC;
    case SC_EXT_MM_PLAY:
      return KEY_MM_PLAY;
    case SC_EXT_MM_STOP:
      return KEY_MM_STOP;
    case SC_EXT_MM_VOL_DOWN:
      return KEY_MM_VOL_DOWN;
    case SC_EXT_MM_VOL_UP:
      return KEY_MM_VOL_UP;
    case SC_EXT_MM_WWW_HOME:
      return KEY_MM_WWW_HOME;
    case SC_EXT_KEYPAD_ENTER:
      return KEY_KEYPAD_ENTER;
    case SC_EXT_KEYPAD_DIV:
      return KEY_KEYPAD_DIV;
    case SC_EXT_RIGHT_CTRL:
      return KEY_RIGHT_CTRL;
    case SC_EXT_RIGHT_ALT:
      return KEY_RIGHT_ALT;
    case SC_EXT_HOME:
      return KEY_HOME;
    case SC_EXT_END:
      return KEY_END;
    case SC_EXT_PAGEUP:
      return KEY_PAGEUP;
    case SC_EXT_PAGEDOWN:
      return KEY_PAGEDOWN;
    case SC_EXT_INSERT:
      return KEY_INSERT;
    case SC_EXT_DELETE:
      return KEY_DELETE;
    case SC_EXT_ARROW_UP:
      return KEY_ARROW_UP;
    case SC_EXT_ARROW_DOWN:
      return KEY_ARROW_DOWN;
    case SC_EXT_ARROW_LEFT:
      return KEY_ARROW_LEFT;
    case SC_EXT_ARROW_RIGHT:
      return KEY_ARROW_RIGHT;
    case SC_EXT_LEFT_GUI:
      return KEY_LEFT_GUI;
    case SC_EXT_RIGHT_GUI:
      return KEY_RIGHT_GUI;
    case SC_EXT_APPS:
      return KEY_APPS;
    case SC_EXT_ACPI_POWER:
      return KEY_ACPI_POWER;
    case SC_EXT_ACPI_SLEEP:
      return KEY_ACPI_SLEEP;
    case SC_EXT_ACPI_WAKE:
      return KEY_ACPI_WAKE;
    case SC_EXT_MM_WWW_SEARCH:
      return KEY_MM_WWW_SEARCH;
    case SC_EXT_MM_WWW_FAV:
      return KEY_MM_WWW_FAV;
    case SC_EXT_MM_WWW_REFRESH:
      return KEY_MM_WWW_REFRESH;
    case SC_EXT_MM_WWW_STOP:
      return KEY_MM_WWW_STOP;
    case SC_EXT_MM_WWW_FORWARD:
      return KEY_MM_WWW_FORWARD;
    case SC_EXT_MM_WWW_BACK:
      return KEY_MM_WWW_BACK;
    case SC_EXT_MM_MY_COMPUTER:
      return KEY_MM_MY_COMPUTER;
    case SC_EXT_MM_EMAIL:
      return KEY_MM_EMAIL;
    case SC_EXT_MM_MEDIA_SELECT:
      return KEY_MM_MEDIA_SELECT;
    default:
      return 0;
  }
}

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

    if (scancode == 0xE0) {
      kbd_extended = 1;
    } else {
      uint8_t key = scancodeToKey(scancode);
      if (kbd_extended) {
        key = getExtendedKey(key);
        kbd_extended = 0;
      }

      kbd_state[key] = isRelease(scancode) ? 0 : 1;
    }

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
    if (kbd_extended) {
      key = getExtendedKey(key);
      kbd_extended = 0;
    }

    if (scancode == 0xE0) {
      kbd_extended = 1;
    } else if ((isSpecial(key) != 0) || isRelease(scancode)) {
      if (key == KEY_CAPSLOCK) {
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
