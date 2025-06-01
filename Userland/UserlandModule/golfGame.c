#include <kbd.h>
#include <stdint.h>
#include <vga.h>


#define T 1
#define VMAX 10

//la necesite la robe de vga, utils.h estaria bueno
#define abs(x) ((x) > 0 ? (x) : -(x))
#define signo(x) ((x > 0) ? 1 : -1)
#define sqr(x) ((x) * (x))
#define chaeckMaxv(x) (x > VMAX ? VMAX : (x < -VMAX ? -VMAX : x))
//


typedef struct {
  float x;
  float y;

  float vx;
  float vy;

  int size;//pensado para ser un radio, se puede usar para suplantar masa

  float mass;// para colisions y podria afectar el gama
  // si usamos la inversa de la masa nos ahoramos la division de float

  float gama;// friction/arraste // NO es exacto, es una idea mas simple

  color_t color;
} physicsObject_t;

typedef struct {
  float x;
  float y;

  int size;
  float incline;

} enviroment_t;


typedef struct {// por alguna extrana rason no puedo usar floats aca
  int x;
  int y;
} vector_t;


void drawObject(physicsObject_t *obj
) {// TODO, hace falta algo para dibujar un circulo
  vga_rect(
    (obj->x - obj->size), (obj->y - obj->size), (obj->x + obj->size),
    (obj->y + obj->size), obj->color, 0
  );
}
void drawHole(enviroment_t *env) {
  vga_rect(
    (env->x - env->size), (env->y - env->size), (env->x + env->size),
    (env->y + env->size), 0xff0000ff, 0
  );
}


/*
*   Reads player input and makes a vector of it 
*   Note: onely 8 directions
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
  vector_t v0 = {0};
  accelerateObject(obj, &v0);
}

/*
* Aplica "aceleracion" y actualiza el estado 
*/
void accelerateObject(physicsObject_t *obj, vector_t *dir) {
  float oldvx = obj->vx;// es para que no oscile dont worry about it
  float oldvy = obj->vy;
  float oldx = obj->x;//para bounds
  float oldy = obj->y;

  //add drag
  obj->vx -= (obj->vx * obj->gama);
  obj->vy -= (obj->vy * obj->gama);
  //make sure that drag doesnt push the other way
  if ((oldvx * obj->vx) < 0) { obj->vx = 0; }//tiene que solo aplicar al gama
  if ((oldvy * obj->vy) < 0) { obj->vy = 0; }

  //add velocity
  obj->vx += (dir->x);// * obj->mass);
  obj->vy += (dir->y);// * obj->mass);
  //check max vel
  obj->vx = (chaeckMaxv(obj->vx));
  obj->vy = (chaeckMaxv(obj->vy));

  //update pos
  obj->x += obj->vx * T;
  obj->y += obj->vy * T;
  //che maxBounds
  if ((obj->x - obj->size) < 0 || (obj->x + obj->size) > VGA_WIDTH) {
    obj->x = oldx;
    obj->vx = -(obj->vx);
  }
  if ((obj->y - obj->size) < 0 || (obj->y + obj->size) > VGA_HEIGHT) {
    obj->y = oldy;
    obj->vy = -(obj->vy);
  }
}

/*
* checks if a colition happens and applyes a repeling vel
*/
void doCollision(physicsObject_t *a, physicsObject_t *b) {

  vector_t dir = {0};
  checkCollision(a, b, &dir);
  if ((dir.x != 0) || (dir.y != 0)) {

    vector_t dirb = dir;
    dirb.x *= b->mass;
    dirb.y *= b->mass;
    accelerateObject(b, &dirb);

    dir.x = -(dir.x * a->mass);
    dir.y = -(dir.y * b->mass);
    accelerateObject(a, &dir);
  }
}
/*
* asumiendo que son circulos 
* retorna el vetor de 'a' a 'b'
*/ //notalolo: final menos inicial
void checkCollision(physicsObject_t *a, physicsObject_t *b, vector_t *dir) {

  float difx = b->x - a->x;
  float dify = b->y - a->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(b->size + a->size)) {
    dir->x = difx;
    dir->y = dify;
  }
}

/*
* checks if obj is in a hole or mount and applyes a apropiate vel
*/
void doEnviroment(enviroment_t *env, physicsObject_t *obj) {
  vector_t dir = {0};
  checkEnviroment(env, obj, &dir);
  if ((dir.x != 0) || (dir.y != 0)) {
    obj->vx += dir.x * env->incline;
    obj->vy += dir.y * env->incline;
  }
}
/* //IDEM checkColition
* asumiendo que son circulos 
* retorna el vetor de 'env' a 'obj'
*/
void checkEnviroment(enviroment_t *env, physicsObject_t *obj, vector_t *dir) {
  float difx = env->x - obj->x;
  float dify = env->y - obj->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(env->size + obj->size)) {
    dir->x = signo(difx);
    dir->y = signo(dify);
  }
}


/*
* Setup y main game loop
*/
int gg_startGame() {

  physicsObject_t mc = {0};
  mc.color = 0xffFF0080;
  mc.x = VGA_WIDTH / 2;
  mc.y = VGA_HEIGHT / 2;
  mc.gama = 0.1;
  mc.size = 20;
  mc.mass = 0.1;


  physicsObject_t ball = {0};
  ball.color = 0xffFF00A0;
  ball.x = VGA_WIDTH / 4;
  ball.y = VGA_HEIGHT / 2;
  ball.gama = 0.05;
  ball.size = 10;
  ball.mass = 1;

  enviroment_t hole = {0};
  hole.x = 900;
  hole.y = 200;
  hole.size = 100;
  hole.incline = 0.5;

  while (1) {
    updateObject(&ball);
    doCollision(&mc, &ball);
    doEnviroment(&hole, &mc);
    doEnviroment(&hole, &ball);
    vector_t input = readImputs();
    accelerateObject(&mc, &input);


    vga_clear(0xFF00FF00);
    drawHole(&hole);
    drawObject(&mc);
    drawObject(&ball);
    vga_present();
  }
  return 0;
}