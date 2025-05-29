#include <kbd.h>
#include <stdint.h>
#include <vga.h>

/* / TODO ver donde va esto pq aca seguro que no ///
typedef struct {
  uint16_t pos[2];
  uint16_t v[2];
  uint16_t a[2];
} player_t;

for (int i = 0; i < 2; i++) {
  mc.v[1] += mc.a[i] * t;
  mc.pos[i] += mc.a[i] * t * t + mc.v[i] * t;
}


player_t mc = {VGA_WIDTH / 2, VGA_HEIGHT / 2, 0};

void drawPlayer() {// pos[0] es x y pos[1] es y
  vga_rect(mc.pos[0], mc.pos[1], mc.pos[0] + 10, mc.pos[1] + 10, 0xffFF0080, 0);
  //   vga_rect(mc.x, mc.y, mc.x + 10, mc.y + 10, 0xffFF0080, 0);
}

*/ /////////////////////////////////////////////////

/*
* standard algo[x,y], si ponemos z es uno mas y chau
*/
typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t vx;
  uint16_t vy;
  uint16_t ax;
  uint16_t ay;
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

  uint16_t t = 1;
  uint16_t gama = 1;

  kbd_pollEvents();

  // y
  if (kbd_keydown(KEY_ARROW_UP)) {
    mc.ay = ((mc.ay - 1) < -1) ? -1 : (mc.ay - 1);
  }
  if (kbd_keydown(KEY_ARROW_DOWN)) {
    mc.ay = ((mc.ay + 1) > 1) ? 1 : (mc.ay + 1);
  }

  //x
  if (kbd_keydown(KEY_ARROW_RIGHT) || kbd_keydown(KEY_ARROW_LEFT)) {
    if (kbd_keydown(KEY_ARROW_RIGHT)) {
      mc.ax = ((mc.ax + 1) > 1) ? 1 : (mc.ax + 1);
    }
    if (kbd_keydown(KEY_ARROW_LEFT)) {
      mc.ax = ((mc.ax - 1) < -1) ? -1 : (mc.ax - 1);
    }
  } else {
    mc.ax -= (mc.vx * gama);
  }


  mc.vx += mc.ax * t;
  mc.vy += mc.ay * t;

  mc.x += mc.ax * t * t + mc.vx * t;
  mc.y += mc.ay * t * t + mc.vy * t;
}

int gg_startGame() {
  while (1) {
    readImputs();
    vga_clear(0xFF00FF00);
    drawPlayer();
    vga_present();
  }
  return 0;
}