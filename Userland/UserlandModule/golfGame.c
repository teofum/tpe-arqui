#include <kbd.h>
#include <stdint.h>
#include <vga.h>


#define T 1
#define VMAX 10

//la necesite la robe de vga, utils.h estaria bueno
#define abs(x) ((x) > 0 ? (x) : -(x))
#define signo(x) ((x >= 0) ? 1 : -1)
#define sqr(x) ((x) * (x))
#define chaeckMaxv(x) (x > VMAX ? VMAX : (x < -VMAX ? -VMAX : x))
//


typedef struct {
  float x;
  float y;

  float vx;
  float vy;

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

  //add drag
  obj->vx -= (obj->vx * obj->gama);
  obj->vy -= (obj->vy * obj->gama);

  //make sure that gamma doesnt push the other way
  if ((oldvx * obj->vx) < 0) { obj->vx = 0; }//tiene que solo aplicar al gama
  if ((oldvy * obj->vy) < 0) { obj->vy = 0; }

  //check max vel
  obj->vx = (chaeckMaxv(obj->vx));
  obj->vy = (chaeckMaxv(obj->vy));

  //update pos
  obj->x += obj->vx * T;
  obj->y += obj->vy * T;
}


/*
* Aplica aceleracion y actualiza el estado 
*/
void accelerateObject(physicsObject_t *obj, vector_t *dir) {
  float oldvx = obj->vx;// es para que no oscile dont worry about it
  float oldvy = obj->vy;


  //add drag
  obj->vx -= (obj->vx * obj->gama);
  obj->vy -= (obj->vy * obj->gama);

  //make sure that drag doesnt push the other way
  if ((oldvx * obj->vx) < 0) { obj->vx = 0; }//tiene que solo aplicar al gama
  if ((oldvy * obj->vy) < 0) { obj->vy = 0; }

  //add velocity
  obj->vx += dir->x;
  obj->vy += dir->y;

  //check max vel
  obj->vx = (chaeckMaxv(obj->vx));
  obj->vy = (chaeckMaxv(obj->vy));

  //update pos
  obj->x += obj->vx * T;
  obj->y += obj->vy * T;
}


/*
* checks if a colition happens and applyes a repelinf accel
*/
void doCollision(physicsObject_t *a, physicsObject_t *b) {

  vector_t dir = {0};
  checkCollision(a, b, &dir);
  if ((dir.x != 0) || (dir.y != 0)) {
    accelerateObject(b, &dir);
    dir.x = -dir.x * a->size;
    dir.y = -dir.y * b->size;
    accelerateObject(a, &dir);
  } else {
    return;
  }
}

/*
* asumiendo que son circulos 
* retorna el vetor de 'a' a 'b'
*/ //nota: final menos inicial
void checkCollision(physicsObject_t *a, physicsObject_t *b, vector_t *dir) {

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
    vector_t input = readImputs();
    accelerateObject(&mc, &input);
    updateObject(&ball);
    doCollision(&mc, &ball);

    vga_clear(0xFF00FF00);
    drawObject(&mc);
    drawObject(&ball);
    vga_present();
  }
  return 0;
}