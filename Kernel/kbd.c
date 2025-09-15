#include <stdint.h>
#include <string.h>

#include <kbd.h>
#include <string.h>

#define scancode_to_key(x) ((x) & 0x7f)
#define is_release(x) ((x) & 0x80)
#define is_special(x)                                                          \
  (((x) == KEY_LEFT_CTRL) || ((x) == KEY_RIGHT_CTRL) ||                        \
   ((x) == KEY_LEFT_SHIFT) || ((x) == KEY_RIGHT_SHIFT) ||                      \
   ((x) == KEY_LEFT_ALT) || ((x) == KEY_RIGHT_ALT) || ((x) == KEY_LEFT_GUI) || \
   ((x) == KEY_RIGHT_GUI) || ((x) == KEY_CAPSLOCK))

#define next(x) x = (x + 1) % KBD_BUFFER_SIZE

// Macros for brevity so we don't break the table formatting into a billion lines
#define UP charcode(KEY_ARROW_UP)
#define DN charcode(KEY_ARROW_DOWN)
#define LFT charcode(KEY_ARROW_LEFT)
#define RGT charcode(KEY_ARROW_RIGHT)

#define SC_EXT_HEADER 0xE0

#define NUM_KEYS 128
#define STATE_QWORDS (NUM_KEYS >> 6)

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

typedef uint64_t kbd_state_t[STATE_QWORDS];

static kbd_buffer_t kbd_buffer = {
  .data = {0},
  .write_pos = 0,
  .read_pos = 0,
};

static kbd_state_t kbd_state = {0};
static kbd_state_t kbd_last_state = {0};

static uint8_t kbd_capslock = 0;
static uint8_t kbd_extended = 0;

static kbd_event_type_t kbd_eventmask = KBD_EV_PRESS;

static int kbd_key2ascii[128] = {
  -1,  0x1B, '1',  '2', '3',  '4', '5', '6',  '7', '8', '9', '0', '-',
  '=', '\b', '\t', 'q', 'w',  'e', 'r', 't',  'y', 'u', 'i', 'o', 'p',
  '[', ']',  '\n', 0,   'a',  's', 'd', 'f',  'g', 'h', 'j', 'k', 'l',
  ';', '\'', '`',  0,   '\\', 'z', 'x', 'c',  'v', 'b', 'n', 'm', ',',
  '.', '/',  0,    '*', 0,    ' ', 0,   0,    0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   '7', '8',  '9', '-', '4', '5', '6',
  '+', '1',  '2',  '3', '0',  '.', 0,   0,    0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   0,   '\n', '/', 0,   0,   0,   0,
  0,   0,    0,    0,   UP,   DN,  LFT, RGT,  0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,    0,   0,   0,    0,   0,   0,
};

static int kbd_key2ascii_shift[128] = {
  -1,  0x1B, '!',  '@', '#', '$', '%', '^',  '&', '*', '(', ')', '_',
  '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T',  'Y', 'U', 'I', 'O', 'P',
  '{', '}',  '\n', 0,   'A', 'S', 'D', 'F',  'G', 'H', 'J', 'K', 'L',
  ':', '"',  '~',  0,   '|', 'Z', 'X', 'C',  'V', 'B', 'N', 'M', '<',
  '>', '?',  0,    '*', 0,   ' ', 0,   0,    0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,   0,   '7', '8',  '9', '-', '4', '5', '6',
  '+', '1',  '2',  '3', '0', '.', 0,   0,    0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,   0,   0,   '\n', '/', 0,   0,   0,   0,
  0,   0,    0,    0,   UP,  DN,  LFT, RGT,  0,   0,   0,   0,   0,
  0,   0,    0,    0,   0,   0,   0,   0,    0,   0,   0,
};

static inline uint64_t get_key_state(kbd_state_t state, uint8_t key) {
  uint8_t qword = key >> 6;
  uint64_t mask = 0x1ull << (key & 0x3F);

  return state[qword] & mask ? 1 : 0;
}

static inline void
set_key_state(kbd_state_t state, uint8_t key, uint8_t value) {
  uint8_t qword = key >> 6;
  uint64_t mask = 0x1ull << (key & 0x3F);

  if (value) {
    state[qword] |= mask;
  } else {
    state[qword] &= ~mask;
  }
}

static uint8_t get_extended_key(uint8_t key) {
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
void kbd_add_key_event(uint8_t sc) {
  kbd_buffer.data[kbd_buffer.write_pos] = sc;
  next(kbd_buffer.write_pos);

  // If we ran into the start of the queue, get rid of the older events
  if (kbd_buffer.write_pos == kbd_buffer.read_pos) next(kbd_buffer.read_pos);
  return;
}

/*
 * Consume the next event in the queue and update keyboard state.
 * Returns nonzero if the event should be handled, 0 if it should be ignored.
 */
static inline int kbd_next_event(kbd_event_t *ev) {
  // Get the next event and advance queue pointer
  uint8_t scancode = kbd_buffer.data[kbd_buffer.read_pos];
  next(kbd_buffer.read_pos);

  // Handle special byte indicating two-part scancode
  if (scancode == SC_EXT_HEADER) {
    kbd_extended = 1;
    return 0;
  }

  // Get the keycode
  uint8_t key = scancode_to_key(scancode);

  // If we're in extended mode, convert to an extended keycode
  if (kbd_extended) {
    key = get_extended_key(key);
    kbd_extended = 0;
  }

  // Update keyboard state
  set_key_state(kbd_state, key, is_release(scancode) ? 0 : 1);

  // Special handling for caps lock, it acts as a toggle
  if (key == KEY_CAPSLOCK && !is_release(scancode))
    kbd_capslock = !kbd_capslock;

  // Return whether event should be handled
  int handle =
    !is_special(key) && (is_release(scancode) ? (kbd_eventmask & KBD_EV_RELEASE)
                                              : (kbd_eventmask & KBD_EV_PRESS));

  // Set event properties if not null
  if (handle && ev != NULL) {
    ev->key = key;
    ev->is_released = is_release(scancode);
    ev->alt = get_key_state(kbd_state, KEY_LEFT_ALT);
    ev->alt_r = get_key_state(kbd_state, KEY_RIGHT_ALT);
    ev->ctrl = get_key_state(kbd_state, KEY_LEFT_CTRL);
    ev->ctrl_r = get_key_state(kbd_state, KEY_RIGHT_CTRL);
    ev->shift = get_key_state(kbd_state, KEY_LEFT_SHIFT);
    ev->shift_r = get_key_state(kbd_state, KEY_RIGHT_SHIFT);
    ev->gui = get_key_state(kbd_state, KEY_LEFT_GUI);
    ev->gui_r = get_key_state(kbd_state, KEY_RIGHT_GUI);
    ev->capslock = kbd_capslock;
  }

  return handle;
}

void kbd_poll_events() {
  memcpy(kbd_last_state, kbd_state, sizeof(kbd_state));

  while (kbd_buffer.read_pos != kbd_buffer.write_pos) { kbd_next_event(NULL); }
}

int kbd_keydown(uint8_t key) { return get_key_state(kbd_state, key); }

int kbd_keypressed(uint8_t key) {
  return (get_key_state(kbd_state, key) && !get_key_state(kbd_last_state, key));
}

int kbd_keyreleased(uint8_t key) {
  return (!get_key_state(kbd_state, key) && get_key_state(kbd_last_state, key));
}

kbd_event_t kbd_get_key_event() {
  kbd_event_t event = {0};
  event.key = 0;

  while (kbd_buffer.read_pos != kbd_buffer.write_pos) {
    uint8_t key;
    if (kbd_next_event(&event)) return event;
  }

  return event;
}

int kbd_getchar() {
  kbd_event_t ev = kbd_get_key_event();

  int is_shifted = ev.shift || ev.shift_r || ev.capslock;
  return is_shifted ? kbd_key2ascii_shift[ev.key] : kbd_key2ascii[ev.key];
}
