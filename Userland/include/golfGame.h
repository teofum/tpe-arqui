#ifndef GOLF_GAME
#define GOLF_GAME 0

#include <fpmath.h>
#include <kbd.h>
#include <stdint.h>
#include <vga.h>

#define T 1
//afecta que tanto v se aplica por frame,                                    \
//mas bajo se mueve mas lento pero va mas fluido
#define VMAX 10
#define TURNS_SPEED 0.01

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

  float gama;// friction/arraste // NO es exacto, es una idea mas simple

  float angle;

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


int gg_startGame();

#endif