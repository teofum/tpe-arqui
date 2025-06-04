#include <fpmath.h>
#include <golfGame.h>
#include <graphics.h>
#include <stddef.h>
#include <syscall.h>
#include <vga.h>

#define deg2rad(x) ((x) / 180.0f * M_PI)

uint8_t *titlescreenLogo = (uint8_t *) 0x3000000;

extern const char *obj_capybase;
extern const char *obj_capyface;
extern const char *obj_capyclub;

float3 v_base[150];
float3 n_base[150];
uint32_t vi_base[280 * 3];
uint32_t ni_base[280 * 3];

float3 v_face[23];
float3 n_face[23];
uint32_t vi_face[20 * 3];
uint32_t ni_face[20 * 3];

float3 v_club[12];
float3 n_club[12];
uint32_t vi_club[20 * 3];
uint32_t ni_club[20 * 3];

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

static void showTitleScreen() {
  float a = 0.0f, capyAngle = 0.0f;

  /*
   * Graphics setup
   */

  // Set half render resolution for speed
  gfx_res_t renderRes = GFX_RES_HALF;
  // gfx_setRenderResolution(renderRes);

  // Set up view and projection matrices
  float3 pos = {0, 1, 3.5f};
  float3 target = {0, 0, 0};
  float3 up = {0, 1, 0};

  float4x4 view = mat_lookat(pos, target, up);
  gfx_setMatrix(GFX_MAT_VIEW, &view);

  float fovDegrees = 75.0f;
  float4x4 projection =
    mat_perspective(deg2rad(fovDegrees), 4.0f / 3.0f, 0.1f, 10.0f);
  gfx_setMatrix(GFX_MAT_PROJECTION, &projection);

  // Set up lighting
  float3 light = {-1, 3, -1};
  float3 lightcolor = {2, 2, 2};
  float3 ambient = vmuls(lightcolor, 0.5f);

  gfx_light_t lightType = GFX_LIGHT_DIRECTIONAL;
  gfx_setLightType(lightType);
  gfx_setLight(GFX_LIGHT_POSITION, &light);
  gfx_setLight(GFX_AMBIENT_LIGHT, &ambient);
  gfx_setLight(GFX_LIGHT_COLOR, &lightcolor);

  // Load the capybara models
  uint32_t pc_base, pc_face, pc_club;
  gfx_parseObj(obj_capybase, v_base, n_base, vi_base, ni_base, &pc_base);
  gfx_parseObj(obj_capyface, v_face, n_face, vi_face, ni_face, &pc_face);
  gfx_parseObj(obj_capyclub, v_club, n_club, vi_club, ni_club, &pc_club);

  float3 color_base = {0.232f, 0.115f, 0.087f};
  float3 color_face = {0.082f, 0.021f, 0.001f};
  float3 color_club = {0.706f, 0.706f, 0.706f};

  /*
   * Draw loop
   */
  kbd_event_t ev = {0};
  while (!ev.key) {
    // Clear graphics frame and depth buffers
    gfx_clear(0);

    // Pass the graphics framebuffer to the VGA driver for 2D drawing
    // This lets us draw shapes behind the 3d graphics
    vga_setFramebuffer(gfx_getFramebuffer());

    // Draw a nice background
    // Sky
    vga_gradient(
      0, 0, VGA_WIDTH - 1, (VGA_HEIGHT >> 1) - 1, colors(0x1a32e6, 0x07d0f8),
      VGA_GRAD_V
    );
    vga_shade(
      0, 0, VGA_WIDTH - 1, (VGA_HEIGHT >> 1) - 1, 0x50000080, VGA_ALPHA_BLEND
    );

    // Grass
    vga_gradient(
      0, VGA_HEIGHT >> 1, VGA_WIDTH - 1, VGA_HEIGHT - 1,
      colors(0x83fa00, 0x008f00), VGA_GRAD_V
    );

    // 3D graphics
    // Create a model matrix and set it
    float4x4 model = mat_rotationY(capyAngle);
    model = mmul(model, mat_translation(0, -1.3, 0));
    gfx_setMatrix(GFX_MAT_MODEL, &model);

    // Draw the capybara
    gfx_drawPrimitivesIndexed(
      v_face, n_face, vi_face, ni_face, pc_face, color_face
    );
    gfx_drawPrimitivesIndexed(
      v_base, n_base, vi_base, ni_base, pc_base, color_base
    );
    gfx_drawPrimitivesIndexed(
      v_club, n_club, vi_club, ni_club, pc_club, color_club
    );

    // Present gfx buffer to VGA main framebuffer
    gfx_present();

    // Restore the default VGA framebuffer to draw on top of the 3D graphics
    vga_setFramebuffer(NULL);

    // Draw the title logo (it floats!)
    vga_bitmap(256, 128 - 18 * sin(a), titlescreenLogo, 2, VGA_ALPHA_BLEND);

    // Draw some text
    vga_text(424, 500, "Press any key to start", 0xffffff, 0, VGA_TEXT_BG);
    vga_text(824, 736, "(c) 1998 TONKATSU GAMES", 0xffffff, 0, 0);

    // Draw everything to screen
    vga_present();

    // Update vars and get keyboard event
    a += 0.01f;
    if (a > M_PI) a -= 2.0f * M_PI;
    capyAngle -= 0.05f;
    if (capyAngle < -M_PI) capyAngle += 2.0f * M_PI;
    ev = kbd_getKeyEvent();
  }
}

/*
* Setup y main game loop
*/
int gg_startGame() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  // Display the title screen
  showTitleScreen();

  // TODO: player select screen (1P - 2P)

  /*
   * Set up game objects
   */
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

  /*
   * Game loop
   */
  while (1) {
    // Update keyboard input
    kbd_pollEvents();

    // Update physics
    updatePlayerTank(&mc);
    updateObject(&mc);
    // updateObject(&ball);
    // doCollision(&mc, &ball);
    // doEnviroment(&hole, &mc);
    // doEnviroment(&hole, &ball);

    // Draw game
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
