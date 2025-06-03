#include <fpmath.h>
#include <golfGame.h>
#include <syscall.h>

void drawObject(
  physicsObject_t *obj
) {// TODO, hace falta algo para dibujar un circulo
  vga_rect(
    (obj->x - obj->size), (obj->y - obj->size), (obj->x + obj->size),
    (obj->y + obj->size), obj->color, 0
  );

  vector_t p;
  p.x = obj->x + 20.0f * cos(obj->angle);
  p.y = obj->y + 20.0f * sin(obj->angle);
  vga_line(obj->x, obj->y, p.x, p.y, 0xffffff, 0);
}
void drawHole(enviroment_t *env) {
  vga_rect(
    (env->x - env->size), (env->y - env->size), (env->x + env->size),
    (env->y + env->size), 0xff0000ff, 0
  );
}

/*
* Aplica "aceleracion" y actualiza el estado 
*/
void accelerateObject(physicsObject_t *obj, vector_t *dir) {
  // Add velocity
  obj->vx += dir->x;
  obj->vy += dir->y;

  // Cap velocity
  // TODO this should be a parameter for different objects
  float v = sqrt(sqr(obj->vx) + sqr(obj->vy));
  if (v > VMAX) {
    float factor = VMAX / v;
    obj->vx *= factor;
    obj->vy *= factor;
  }
}

/*
*   Reads player input and accelerates it
*   /aka if you use this, the player doesnt need to be
*   updated 
*   Note: onely 8 directions
*/
void updatePlayerDirectional(physicsObject_t *obj) {
  kbd_pollEvents();
  vector_t arrowKeys = {0};
  int up = kbd_keydown(KEY_ARROW_UP);
  int down = kbd_keydown(KEY_ARROW_DOWN);
  int right = kbd_keydown(KEY_ARROW_RIGHT);
  int left = kbd_keydown(KEY_ARROW_LEFT);

  if (up || down) { arrowKeys.y = (down - up); }
  if (left || right) { arrowKeys.x = (right - left); }

  accelerateObject(obj, &arrowKeys);
}

/*
* tank controls for player, accelerates it
* /aka if you use this, the player doesnt need to be
* updated 
*/
void updatePlayerTank(physicsObject_t *obj) {
  kbd_pollEvents();
  int up = kbd_keydown(KEY_ARROW_UP);
  int down = kbd_keydown(KEY_ARROW_DOWN);
  int right = kbd_keydown(KEY_ARROW_RIGHT);
  int left = kbd_keydown(KEY_ARROW_LEFT);

  if (right) {
    obj->angle += TURNS_SPEED;
    if (obj->angle > M_PI) obj->angle -= 2.0f * M_PI;
  }
  if (left) {
    obj->angle -= TURNS_SPEED;
    if (obj->angle < -M_PI) obj->angle += 2.0f * M_PI;
  }
  if (up) {
    vector_t dir;
    // TODO make acceleration a constant
    dir.x = 0.5f * cos(obj->angle);
    dir.y = 0.5f * sin(obj->angle);
    accelerateObject(obj, &dir);
  }
}

/*
* Actualiza el estado sin aplicarle una aceleracion
*/
void updateObject(physicsObject_t *obj) {
  float oldvx = obj->vx;// es para que no oscile dont worry about it
  float oldvy = obj->vy;

  float oldx = obj->x;//para bounds
  float oldy = obj->y;

  //add drag
  obj->vx -= obj->vx * obj->drag;
  obj->vy -= obj->vy * obj->drag;

  //update pos
  obj->x += obj->vx * T;
  obj->y += obj->vy * T;

  // //che maxBounds
  if ((obj->x - obj->size) < 0 || (obj->x + obj->size) > VGA_WIDTH - 1) {
    obj->x = oldx;
    obj->vx = -(obj->vx);
  }
  if ((obj->y - obj->size) < 0 || (obj->y + obj->size) > VGA_HEIGHT - 1) {
    obj->y = oldy;
    obj->vy = -(obj->vy);
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
    //TODO, habria que normalizarlo o algo asi/ ///////////////////////
    dir->x = (difx);
    dir->y = (dify);
  }
}

/*
* checks if a colition happens and applyes a repeling vel
*/
void doCollision(physicsObject_t *a, physicsObject_t *b) {
  float va = abs(a->vx) + abs(a->vy);
  float vb = abs(b->vx) + abs(b->vy);

  vector_t dir = {0};
  checkCollision(a, b, &dir);
  if ((dir.x != 0) || (dir.y != 0)) {
    vector_t dirb = dir;//TODO: esto esta andando raro
    dirb.x *= (b->mass * (va + vb));
    dirb.y *= (b->mass * (va + vb));
    accelerateObject(b, &dirb);

    dir.x *= -((a->mass * (va + vb)));
    dir.y *= -((a->mass * (va + vb)));
    accelerateObject(a, &dir);
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

/*
* Setup y main game loop
*/
int gg_startGame() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  physicsObject_t mc = {0};
  mc.color = 0xFF0080;
  mc.x = VGA_WIDTH * 0.5f;
  mc.y = VGA_HEIGHT * 0.5f;
  mc.drag = 0.03f;
  mc.size = 20;
  mc.mass = 0.1f;
  mc.angle = 0.0f;

  physicsObject_t ball = {0};
  ball.color = 0xffFF00A0;
  ball.x = VGA_WIDTH * 4.0f;
  ball.y = VGA_HEIGHT * 4.0f;
  ball.drag = 0.05;
  ball.size = 10;
  ball.mass = 1;

  enviroment_t hole = {0};
  hole.x = 900;
  hole.y = 200;
  hole.size = 100;
  hole.incline = 0.5;

  while (1) {
    // updatePlayerDirectional(&mc);
    updatePlayerTank(&mc);
    updateObject(&mc);
    // updateObject(&ball);
    // doCollision(&mc, &ball);
    // doEnviroment(&hole, &mc);
    // doEnviroment(&hole, &ball);

    vga_clear(0xFF00FF00);
    // drawHole(&hole);
    drawObject(&mc);
    // drawObject(&ball);
    vga_present();
  }

  // Restore status bar enabled state
  _syscall(SYS_STATUS_SET_ENABLED, statusEnabled);

  return 0;
}
