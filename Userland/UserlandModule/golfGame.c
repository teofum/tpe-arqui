#include <kbd.h>
#include <stdint.h>
#include <vga.h>

//la necesite la robe de vga, utils.h estaria bueno
#define abs(x) ((x) > 0 ? (x) : -(x))
#define sqr(x) ((x) * (x))
#define sqroot(x) (x)//help
//

#define T 0.1

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

typedef struct {// por alguna extrana rason no puedo usar floats aca
  int x;
  int y;
} vector_t;


void drawObject(physicsObject_t *obj) {
  vga_rect(
    (obj->x - obj->size), (obj->y - obj->size), (obj->x + (obj->size * 2)),
    (obj->y + (obj->size * 2)), 0xffFF0080, 0
  );
}

/*
*   updates player info
*/
vector_t readImputs() {
  kbd_pollEvents();
  vector_t arrowKeys = {0};
  int up = kbd_keydown(KEY_ARROW_UP);
  int down = kbd_keydown(KEY_ARROW_DOWN);
  int right = kbd_keydown(KEY_ARROW_RIGHT);
  int left = kbd_keydown(KEY_ARROW_LEFT);

  if (up || down) { arrowKeys.y = (down - up); }
  if (left || right) { arrowKeys.x = (right - left); }

  return arrowKeys;
}

/*
* Actualiza el estado sin aplicarle una aceleracion
*/
void updateObject(physicsObject_t *obj) {
  float oldvx = obj->vx;// es para que no oscile dont worry about it
  float oldvy = obj->vy;

  obj->ax -= (obj->vx * obj->gama);
  obj->ay -= (obj->vy * obj->gama);

  obj->vx += obj->ax * T;
  obj->vy += obj->ay * T;

  obj->x += obj->ax * T * T + obj->vx * T;
  obj->y += obj->ay * T * T + obj->vy * T;

  if ((oldvx * obj->vx) < 0) {
    obj->vx = 0;
    obj->ax = 0;
  }
  if ((oldvy * obj->vy) < 0) {
    obj->vy = 0;
    obj->ay = 0;
  }
}


/*
* Aplica aceleracion y actualiza el estado 
*/
void accelerateObject(physicsObject_t *obj, vector_t *dir) {
  float oldvx = obj->vx;// es para que no oscile dont worry about it
  float oldvy = obj->vy;

  //x
  if (dir->x > 0) {
    obj->ax = ((obj->ax + dir->x) > 10) ? 10 : (obj->ax + dir->x);
  } else if (dir->x < 0) {
    obj->ax = ((obj->ax + dir->x) < -10) ? -10 : (obj->ax + dir->x);
  } else {
    // obj->ax = -(obj->vx * obj->gama);
    obj->ax -= (obj->vx * obj->gama);
  }

  //y
  if (dir->y > 0) {
    obj->ay = ((obj->ay + dir->y) > 10) ? 10 : (obj->ay + dir->y);
  } else if (dir->y < 0) {
    obj->ay = ((obj->ay + dir->y) < -10) ? -10 : (obj->ay + dir->y);
  } else {
    // obj->ay = -(obj->vy * obj->gama);
    obj->ay -= (obj->vy * obj->gama);
  }


  obj->vx += obj->ax * T;
  obj->vy += obj->ay * T;

  obj->x += obj->ax * T * T + obj->vx * T;
  obj->y += obj->ay * T * T + obj->vy * T;

  if ((oldvx * obj->vx) < 0) {
    obj->vx = 0;
    obj->ax = 0;
  }
  if ((oldvy * obj->vy) < 0) {
    obj->vy = 0;
    obj->ay = 0;
  }
}


/*
* checks if a colition happens and applyes a repelinf accel
*/
void doColition(physicsObject_t *a, physicsObject_t *b) {

  vector_t dir = {0};
  checkColition(a, b, &dir);
  if ((dir.x * dir.y) != 0) {
    accelerateObject(b, &dir);
    dir.x = -dir.x;
    dir.y = -dir.y;
    accelerateObject(a, &dir);
  } else {
    return;
  }
}

/*
* asumiendo que son circulos 
* retorna el vetor de 'a' a 'b'
*/ //nota: final menos inicial
void checkColition(physicsObject_t *a, physicsObject_t *b, vector_t *dir) {
  //TODO los bounds estan andando medio sus//////////////////////////////////////////////
  float difx = b->x - a->x;
  float dify = b->y - a->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr < sqr(b->size + a->size)) {
    dir->x = difx;
    dir->y = dify;
  }
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
  ball.gama = 0.05;
  ball.size = 10;

  while (1) {
    vga_clear(0xFF00FF00);

    vector_t input = readImputs();
    accelerateObject(&mc, &input);
    updateObject(&ball);

    doColition(&mc, &ball);
    drawObject(&mc);
    drawObject(&ball);
    vga_present();
  }
  return 0;
}