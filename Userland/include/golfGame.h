#ifndef GOLF_GAME
#define GOLF_GAME 0

#include <fpmath.h>
#include <kbd.h>
#include <stdint.h>
#include <vga.h>

#define T 0.5f
//afecta que tanto v se aplica por frame,                                    \
//mas bajo se mueve mas lento pero va mas fluido
#define VMAX 10.0f
#define TURNS_SPEED 0.1f
#define BRAKING 0.9

//la necesite la robe de vga, utils.h estaria bueno
#define abs(x) ((x) >= 0 ? (x) : -(x))
#define signo(x) (((x) == 0) ? 0 : ((x) > 0 ? 1 : -1))
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

  float drag;// friction/arraste // NO es exacto, es una idea mas simple

  float angle;

  color_t color;
} physicsObject_t;

typedef struct {
  float x;
  float y;

  int size;
  float incline;
} enviroment_t;

typedef struct {
  float x;
  float y;

  int size;
} hole_t;

typedef struct {
  float x;
  float y;
} vector_t;


int gg_startGame();

#endif
