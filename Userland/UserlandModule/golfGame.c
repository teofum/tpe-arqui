#include <kbd.h>
#include <stdint.h>
#include <vga.h>

//la necesite la robe de vga, utils.h estaria bueno
#define abs(x) ((x) > 0 ? (x) : -(x))
#define sqr(x) ((x) * (x))
#define sqroot(x) (x)//help
//

#define T 0.5

typedef struct {
  float x;
  float y;

  float vx;
  float vy;

  float ax;
  float ay;

  int size;//pensado para ser un radio, se puede usar para suplantar masa

  float
    gama;// friction/arraste // NO es exacto, es una idea mas simple, ver si queremos meter masa
} physicsObject_t;

typedef struct {// TODO: hacerlo un bitfield, me olvide como hacerlo
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
} direction_t;


void drawObject(physicsObject_t obj) {
  vga_rect(
    (obj.x - obj.size), (obj.y - obj.size), (obj.x + (obj.size * 2)),
    (obj.y + (obj.size * 2)), 0xffFF0080, 0
  );
}

/*
*   updates player info
*/
direction_t readImputs() {
  kbd_pollEvents();
  direction_t arrowKeys = {0};
  arrowKeys.up = (kbd_keydown(KEY_ARROW_UP));
  arrowKeys.down = kbd_keydown(KEY_ARROW_DOWN);
  arrowKeys.right = kbd_keydown(KEY_ARROW_RIGHT);
  arrowKeys.left = kbd_keydown(KEY_ARROW_LEFT);

  return arrowKeys;
}

/*
* Actualiza el estado sin aplicarle una aceleracion
*/
void updateObject(physicsObject_t *obj) {
  direction_t dir = {0};
  accelerateObject(&obj, &dir);
}


/*
* Aplica aceleracion y actualiza el estado 
*/
void accelerateObject(physicsObject_t *obj, direction_t *dir) {
  //x
  if (dir->right || dir->left) {
    if (dir->right) { obj->ax = ((obj->ax + 1) > 1) ? 1 : (obj->ax + 1); }
    if (dir->left) { obj->ax = ((obj->ax - 1) < -1) ? -1 : (obj->ax - 1); }
  } else {
    obj->ax = -(obj->vx * obj->gama);
  }

  //y
  if (dir->up || dir->down) {
    if (dir->up) { obj->ay = ((obj->ay - 1) < -1) ? -1 : (obj->ay - 1); }
    if (dir->down) { obj->ay = ((obj->ay + 1) > 1) ? 1 : (obj->ay + 1); }
  } else {
    obj->ay = -(obj->vy * obj->gama);
  }


  obj->vx += obj->ax * T;
  obj->vy += obj->ay * T;

  obj->x += obj->ax * T * T + obj->vx * T;
  obj->y += obj->ay * T * T + obj->vy * T;
}

void doColition(physicsObject_t *a, physicsObject_t *b) {

  if (checkColition(a, b)) {}
}

//asumiendo que son circulos
int checkColition(physicsObject_t *a, physicsObject_t *b) {
  float dist = sqroot(sqr(a->x - b->x) + sqr(a->y - b->y));
  return (dist < (a->size + b->size));
}

/*
* Setup y main game loop
*/
int gg_startGame() {

  physicsObject_t mc = {0};
  mc.x = VGA_WIDTH / 2;
  mc.y = VGA_HEIGHT / 2;
  mc.gama = 0.1;
  mc.size = 20;


  physicsObject_t ball = {0};
  ball.x = VGA_WIDTH / 4;
  ball.y = VGA_HEIGHT / 2;
  ball.gama = 0.1;
  ball.size = 10;

  while (1) {
    vga_clear(0xFF00FF00);

    direction_t input = readImputs();
    accelerateObject(&mc, &input);

    drawObject(mc);
    drawObject(ball);
    vga_present();
  }
  return 0;
}