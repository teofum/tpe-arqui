#include <lib.h>
#include <mouse.h>
#include <print.h>
#include <vga.h>

#define MOUSE_CONTROL_PORT 0x64
#define MOUSE_DATA_PORT 0x60

typedef enum {
  MOUSE_READ = 0,
  MOUSE_WRITE = 1,
} mouse_op_t;

static void mouse_wait(mouse_op_t op) {
  uint32_t timeout = 10000;
  while (timeout--) {
    if ((inb(MOUSE_CONTROL_PORT) & (op << 1)) == 1 - op) return;
  }
}

static uint8_t mouse_read() {
  mouse_wait(MOUSE_READ);
  return inb(MOUSE_DATA_PORT);
}

static void mouse_write(uint8_t byte) {
  mouse_wait(MOUSE_WRITE);
  outb(MOUSE_CONTROL_PORT, 0xD4);
  mouse_wait(MOUSE_WRITE);
  outb(MOUSE_DATA_PORT, byte);
}

void mouse_init() {
  // Enable auxiliary PS2 input (mouse)
  mouse_wait(MOUSE_WRITE);
  outb(MOUSE_CONTROL_PORT, 0xA8);

  // Enable interrupts on IRQ12
  mouse_wait(MOUSE_WRITE);
  outb(MOUSE_CONTROL_PORT, 0x20);
  uint8_t status = mouse_read();
  status |= 0x2;
  status &= ~(0x20);
  mouse_wait(MOUSE_WRITE);
  outb(MOUSE_CONTROL_PORT, 0x60);
  mouse_wait(MOUSE_WRITE);
  outb(MOUSE_DATA_PORT, status);

  // Tell the mouse to use default settings
  mouse_write(0xF6);
  mouse_read();// ACK

  // Set resolution to max
  mouse_write(0xE8);
  mouse_write(0x03);
  mouse_read();// ACK

  // Set sample rate
  mouse_write(0xF3);
  mouse_write(100);
  mouse_read();// ACK

  // Enable streaming mode
  mouse_write(0xF4);
  mouse_read();// ACK
}

static int32_t mouse_x = 0;
static int32_t mouse_y = 0;

static void mouse_update(uint8_t data[3]) {
  uint8_t info = data[0];
  if (info & 0b11000000) return;// Overflow bits set, throw away the packet

  int32_t dx = data[1] & 0xff;
  int32_t dy = data[2] & 0xff;

  if (info & (0x1 << 5)) dy |= 0xffffff00;// Negative Y
  if (info & (0x1 << 4)) dx |= 0xffffff00;// Negative X

  // printf("%#08b, %5d %5d\n", info, dx, dy);

  mouse_x += dx;
  mouse_y -= dy;

  if (mouse_x < 0) mouse_x = 0;
  if (mouse_x >= 1024) mouse_x = 1023;
  if (mouse_y < 0) mouse_y = 0;
  if (mouse_y >= 768) mouse_y = 767;

  vga_present();
}

void mouse_handler(uint8_t byte) {
  static uint32_t mouse_cycle = 0;
  static uint8_t mouse_bytes[3] = {0};

  if (mouse_cycle == 0 && !(byte & (0x1 << 3))) return;

  mouse_bytes[mouse_cycle] = byte;
  switch (mouse_cycle) {
    case 0:
    case 1:
      mouse_cycle++;
      break;
    case 2:
      mouse_cycle = 0;
      mouse_update(mouse_bytes);
      break;
    case 3:
      mouse_cycle = 0;
      break;
  }
}

point_t mouse_getpos() {
  return (point_t) {
    .x = mouse_x,
    .y = mouse_y,
  };
}
