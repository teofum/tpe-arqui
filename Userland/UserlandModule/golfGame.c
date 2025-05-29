#include <kbd.h>
#include <stdint.h>
#include <vga.h>

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  float ax;
  float ay;
} player_t;

player_t mc = {VGA_WIDTH / 2, VGA_HEIGHT / 2, 0};

void drawPlayer() {
  //   vga_pixel(mc.pos[0], mc.pos[1], 0xFFFF000F, 0);
  vga_rect(mc.x, mc.y, mc.x + 10, mc.y + 10, 0xffFF0080, 0);
}

/*
*   updates player info
*/
void readImputs() {

  float t =
    0.5;//con esto se puede ajustar la velocidad pero ideal seria liquearlo con algun clock
  float gama =
    0.05;// con esto se ajusta el arrastre, mientras mas alto mas se frena, idealmente entre 1 y 0

  kbd_pollEvents();
  uint8_t up = kbd_keydown(KEY_ARROW_UP);
  uint8_t down = kbd_keydown(KEY_ARROW_DOWN);
  uint8_t right = kbd_keydown(KEY_ARROW_RIGHT);
  uint8_t left = kbd_keydown(KEY_ARROW_LEFT);

  //x
  if (right || left) {
    if (right) { mc.ax = ((mc.ax + 1) > 1) ? 1 : (mc.ax + 1); }
    if (left) { mc.ax = ((mc.ax - 1) < -1) ? -1 : (mc.ax - 1); }
  } else {
    mc.ax = -(mc.vx * gama);
  }

  //y
  if (up || down) {
    if (up) { mc.ay = ((mc.ay - 1) < -1) ? -1 : (mc.ay - 1); }
    if (down) { mc.ay = ((mc.ay + 1) > 1) ? 1 : (mc.ay + 1); }
  } else {
    mc.ay = -(mc.vy * gama);
  }


  mc.vx += mc.ax * t;
  mc.vy += mc.ay * t;

  mc.x += mc.ax * t * t + mc.vx * t;
  mc.y += mc.ay * t * t + mc.vy * t;
}

/*
* Setup y main game loop
*/
int gg_startGame() {
  while (1) {
    readImputs();
    vga_clear(0xFF00FF00);
    drawPlayer();
    vga_present();
  }
  return 0;
}