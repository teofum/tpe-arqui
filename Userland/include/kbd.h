#ifndef KBD_H
#define KBD_H
#include <stdint.h>

typedef enum {
  // Standard scancodes, keycode is equal to the (pressed) scancode for these
  KEY_ESCAPE = 0x01,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,
  KEY_MINUS,
  KEY_EQUAL,
  KEY_BACKSPACE,
  KEY_TAB,
  KEY_Q,
  KEY_W,
  KEY_E,
  KEY_R,
  KEY_T,
  KEY_Y,
  KEY_U,
  KEY_I,
  KEY_O,
  KEY_P,
  KEY_LEFT_BRACKET,
  KEY_RIGHT_BRACKET,
  KEY_RETURN,
  KEY_LEFT_CTRL,
  KEY_A,
  KEY_S,
  KEY_D,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_SEMICOLON,
  KEY_QUOTE,
  KEY_BACKTICK,
  KEY_LEFT_SHIFT,
  KEY_BACKSLASH,
  KEY_Z,
  KEY_X,
  KEY_C,
  KEY_V,
  KEY_B,
  KEY_N,
  KEY_M,
  KEY_COMMA,
  KEY_PERIOD,
  KEY_SLASH,
  KEY_RIGHT_SHIFT,
  KEY_KEYPAD_MULT,
  KEY_LEFT_ALT,
  KEY_SPACE,
  KEY_CAPSLOCK,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_NUM_LOCK,
  KEY_SCROLL_LOCK,
  KEY_KEYPAD_7,
  KEY_KEYPAD_8,
  KEY_KEYPAD_9,
  KEY_KEYPAD_MINUS,
  KEY_KEYPAD_4,
  KEY_KEYPAD_5,
  KEY_KEYPAD_6,
  KEY_KEYPAD_PLUS,
  KEY_KEYPAD_1,
  KEY_KEYPAD_2,
  KEY_KEYPAD_3,
  KEY_KEYPAD_0,
  KEY_KEYPAD_DOT,
  KEY_F11 = 0x57,
  KEY_F12,

  // Extended (two-part scancode) keycodes
  KEY_MM_PREV = 0x59,
  KEY_MM_NEXT,
  KEY_MM_MUTE,
  KEY_MM_CALC,
  KEY_MM_PLAY,
  KEY_MM_STOP,
  KEY_MM_VOL_DOWN,
  KEY_MM_VOL_UP,
  KEY_MM_WWW_HOME,
  KEY_KEYPAD_ENTER,
  KEY_KEYPAD_DIV,
  KEY_RIGHT_CTRL,
  KEY_RIGHT_ALT,
  KEY_HOME,
  KEY_END,
  KEY_PAGEUP,
  KEY_PAGEDOWN,
  KEY_INSERT,
  KEY_DELETE,
  KEY_ARROW_UP,
  KEY_ARROW_DOWN,
  KEY_ARROW_LEFT,
  KEY_ARROW_RIGHT,
  KEY_LEFT_GUI,
  KEY_RIGHT_GUI,
  KEY_APPS,
  KEY_ACPI_POWER,
  KEY_ACPI_SLEEP,
  KEY_ACPI_WAKE,
  KEY_MM_WWW_SEARCH,
  KEY_MM_WWW_FAV,
  KEY_MM_WWW_REFRESH,
  KEY_MM_WWW_STOP,
  KEY_MM_WWW_FORWARD,
  KEY_MM_WWW_BACK,
  KEY_MM_MY_COMPUTER,
  KEY_MM_EMAIL,
  KEY_MM_MEDIA_SELECT,
} keycode_t;

typedef struct {
  uint8_t key;
  uint8_t isReleased;

  uint8_t shift : 1;
  uint8_t shift_r : 1;
  uint8_t ctrl : 1;
  uint8_t alt : 1;
  uint8_t capslock : 1;
} kbd_event_t;


typedef enum {
  // Standard scancodes, keycode is equal to the (pressed) scancode for these
  KEY_ESCAPE = 0x01,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,
  KEY_MINUS,
  KEY_EQUAL,
  KEY_BACKSPACE,
  KEY_TAB,
  KEY_Q,
  KEY_W,
  KEY_E,
  KEY_R,
  KEY_T,
  KEY_Y,
  KEY_U,
  KEY_I,
  KEY_O,
  KEY_P,
  KEY_LEFT_BRACKET,
  KEY_RIGHT_BRACKET,
  KEY_RETURN,
  KEY_LEFT_CTRL,
  KEY_A,
  KEY_S,
  KEY_D,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_SEMICOLON,
  KEY_QUOTE,
  KEY_BACKTICK,
  KEY_LEFT_SHIFT,
  KEY_BACKSLASH,
  KEY_Z,
  KEY_X,
  KEY_C,
  KEY_V,
  KEY_B,
  KEY_N,
  KEY_M,
  KEY_COMMA,
  KEY_PERIOD,
  KEY_SLASH,
  KEY_RIGHT_SHIFT,
  KEY_KEYPAD_MULT,
  KEY_LEFT_ALT,
  KEY_SPACE,
  KEY_CAPSLOCK,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_NUM_LOCK,
  KEY_SCROLL_LOCK,
  KEY_KEYPAD_7,
  KEY_KEYPAD_8,
  KEY_KEYPAD_9,
  KEY_KEYPAD_MINUS,
  KEY_KEYPAD_4,
  KEY_KEYPAD_5,
  KEY_KEYPAD_6,
  KEY_KEYPAD_PLUS,
  KEY_KEYPAD_1,
  KEY_KEYPAD_2,
  KEY_KEYPAD_3,
  KEY_KEYPAD_0,
  KEY_KEYPAD_DOT,
  KEY_F11 = 0x57,
  KEY_F12,

  // Extended (two-part scancode) keycodes
  KEY_MM_PREV = 0x59,
  KEY_MM_NEXT,
  KEY_MM_MUTE,
  KEY_MM_CALC,
  KEY_MM_PLAY,
  KEY_MM_STOP,
  KEY_MM_VOL_DOWN,
  KEY_MM_VOL_UP,
  KEY_MM_WWW_HOME,
  KEY_KEYPAD_ENTER,
  KEY_KEYPAD_DIV,
  KEY_RIGHT_CTRL,
  KEY_RIGHT_ALT,
  KEY_HOME,
  KEY_END,
  KEY_PAGEUP,
  KEY_PAGEDOWN,
  KEY_INSERT,
  KEY_DELETE,
  KEY_ARROW_UP,
  KEY_ARROW_DOWN,
  KEY_ARROW_LEFT,
  KEY_ARROW_RIGHT,
  KEY_LEFT_GUI,
  KEY_RIGHT_GUI,
  KEY_APPS,
  KEY_ACPI_POWER,
  KEY_ACPI_SLEEP,
  KEY_ACPI_WAKE,
  KEY_MM_WWW_SEARCH,
  KEY_MM_WWW_FAV,
  KEY_MM_WWW_REFRESH,
  KEY_MM_WWW_STOP,
  KEY_MM_WWW_FORWARD,
  KEY_MM_WWW_BACK,
  KEY_MM_MY_COMPUTER,
  KEY_MM_EMAIL,
  KEY_MM_MEDIA_SELECT,
} keycode_t;


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
