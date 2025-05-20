#include <kbd.h>
#include <string.h>

typedef enum {
  SC_CTRL = 0x1D, // Left Ctrl
  SC_LSHIFT = 0x2A,
  SC_RSHIFT = 0x36,
  SC_ALT = 0x38, // Left Alt
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

extern uint8_t _kbd_read();
extern uint8_t _kbd_readState();

/*
 * Called by keyboard interrupt handler.
 * Adds a scancode to the buffer, discarding oldest events fi we run out of
 * space.
 */
void kbd_addKeyEvent(uint8_t scancode) {
  kbd_buffer.data[kbd_buffer.writePos] = scancode;
  next(kbd_buffer.writePos);

  // If we ran into the start of the queue, get rid of the older events
  if (kbd_buffer.writePos == kbd_buffer.readPos)
    next(kbd_buffer.readPos);
}

void kbd_pollEvents() {
  memcpy(kbd_lastState, kbd_state, sizeof(kbd_state));

  while (kbd_buffer.readPos != kbd_buffer.writePos) {
    uint8_t scancode = kbd_buffer.data[kbd_buffer.readPos];
    if (scancodeToKey(scancode) == SC_CAPLOCK) { //  togle caplock on press
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
}

// /*
// // Keyboard us,
// // todo: revisar los valores
// // la de specials se podria insertar en las otras
// */
// const char *scancode_to_ascii[128] = {
//     0,    27,  "1", "2",  "3",  "4", "5",  "6",  // 0x00 - 0x07
//     "7",  "8", "9", "0",  "-",  "=", "\b", "\t", // 0x08 - 0x0F
//     "q",  "w", "e", "r",  "t",  "y", "u",  "i",  // 0x10 - 0x17
//     "o",  "p", "[", "]",  "\n", 0,   "a",  "s",  // 0x18 - 0x1F
//     "d",  "f", "g", "h",  "j",  "k", "l",  ";",  // 0x20 - 0x27
//     "\"", "`", 0,   "\\", "z",  "x", "c",  "v",  // 0x28 - 0x2F
//     "b",  "n", "m", ",",  ".",  "/", 0,    "*",  // 0x30 - 0x37
//     0,    " ", 0,   0,    0,    0,   0,    0,    // 0x38 - 0x3F
//     0,    0,   0,   0,    0,    0,   0,    "7",  // 0x40 - 0x47
//     "8",  "9", "-", "4",  "5",  "6", "+",  "1",  // 0x48 - 0x4F
//     "2",  "3", "0", ".",  0,    0,   0,    0,    // 0x50 - 0x57
//     0,    0,   0,   0,    0,    0,   0,    0     // 0x58 - 0x5F
// };
//
// const char *scancode_to_ascii_shifted[128] = {
//     0,    27,  "!", "@", "#",  "$", "%",  "^",  // 0x00 - 0x07
//     "&",  "*", "(", ")", "_",  "+", "\b", "\t", // 0x08 - 0x0F
//     "Q",  "W", "E", "R", "T",  "Y", "U",  "I",  // 0x10 - 0x17
//     "O",  "P", "{", "}", "\n", 0,   "A",  "S",  // 0x18 - 0x1F
//     "D",  "F", "G", "H", "J",  "K", "L",  ":",  // 0x20 - 0x27
//     "\"", "~", 0,   "|", "Z",  "X", "C",  "V",  // 0x28 - 0x2F
//     "B",  "N", "M", "<", ">",  "?", 0,    "*",  // 0x30 - 0x37
//     0,    " ", 0,   0,   0,    0,   0,    0,    // 0x38 - 0x3F
//     0,    0,   0,   0,   0,    0,   0,    "7",  // 0x40 - 0x47
//     "8",  "9", "-", "4", "5",  "6", "+",  "1",  // 0x48 - 0x4F
//     "2",  "3", "0", ".", 0,    0,   0,    0,    // 0x50 - 0x57
//     0,    0,   0,   0,   0,    0,   0,    0     // 0x58 - 0x5F
// };
//

//
// /* Verifica si el buffer está lleno */
// uint8_t isBuffFull(struct buffer *buff) {
//   return buff->write_pos < BUFFER_SIZE;
// }
//
// /* Agrega un carácter al buffer */
// void addCharToBuff(char *c, struct buffer *buff) {
//   buff->data[buff->write_pos] = c;
//   buff->write_pos += 1;
// }
//
// /* Pasa de scan code a string_(:v\)/_*/ // nota: no altera la flag, asiq no
// lo
//                                         // uses en cualquier lado
// char *scancodeToString(uint8_t sc) {
//   if (scancode_to_special[sc] != 0) {
//     return scancode_to_special[sc];
//   } else {
//     return scancode_to_ascii[sc];
//   }
// }
//
// /* Llena el buffer con una combinacion de teclas */
// struct buffer _kbd_readKeyCombo() {
//
//   struct buffer buff = {.data = {0}, .write_pos = 0, .isChar = 1};
//
//   // int presscount=0;
//   char firstKey = _kbd_read();
//   addCharToBuff(scancodeToString(firstKey), &buff);
//
//   for (int i = 1; i < (BUFFER_SIZE - 1); i++) {
//     char currKey = _kbd_read();
//     // presscount++;
//
//     if (currKey & 0x80) { // valida si es un release
//       if ((currKey & firstKey) ==
//           firstKey) { // deberia validar si es el release de la primera tecla
//         // for(int i=0; i<presscount;i++){//vaciar el buffer de releases
//         //   _kbd_read();
//         // }
//         return buff;
//       }
//       // presscount-2;
//     } else if (scancode_to_special[currKey] != 0) { // si es tecla especial
//       buff.isChar = 0;
//       buff.data[i] = scancode_to_special[currKey];
//
//     } else { // si es tecla normal
//       buff.data[i] = scancode_to_ascii[currKey];
//     }
//   }
//   return buff;
// }
//
// /* returns the first key pressed /TESTING ONLY */
// char *_kbd_readString() {
//   struct buffer buff = _kbd_readKeyCombo();
//   return buff.data[0];
// }
//
// /* returns the buffer */
// char **_kbd_readBuffer() {
//   struct buffer buff = _kbd_readKeyCombo();
//   return buff.data;
// }
