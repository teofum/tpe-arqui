#ifndef GOLF_GAME
#define GOLF_GAME 0

#include <fpmath.h>
#include <kbd.h>
#include <stdint.h>
#include <vga.h>

#define VMAX 1.0f
#define TURNS_SPEED 0.01f
#define ACCELERATION 0.2f
#define BRAKING 0.9

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define sign(x) (((x) == 0) ? 0 : ((x) > 0 ? 1 : -1))
#define sqr(x) ((x) * (x))

int gg_startGame();

#endif
