#include "kbd.h"
#include <fpmath.h>
#include <golfGame.h>
#include <graphics.h>
#include <stddef.h>
#include <syscall.h>
#include <vga.h>

#define deg2rad(x) ((x) / 180.0f * M_PI)

#define TITLE_TEXT_BLINK_MS 500

#define FIELD_WIDTH 15.0f
#define FIELD_HEIGHT 10.0f

#define TERRAIN_SIZE_X 15
#define TERRAIN_SIZE_Y 10
#define TERRAIN_SIZE_UNITS_X (FIELD_WIDTH / TERRAIN_SIZE_X)
#define TERRAIN_SIZE_UNITS_Y (FIELD_HEIGHT / TERRAIN_SIZE_Y)

#define VMAX 1.0f
#define TURNS_SPEED 0.01f
#define ACCELERATION 0.01f
#define GRAVITY 0.1f
#define BRAKING 0.9

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define sign(x) (((x) == 0) ? 0 : ((x) > 0 ? 1 : -1))
#define sqr(x) ((x) * (x))

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define height(t, xx, yy) (t->v[(xx)][(yy)].y)

#define lerp(a, b, t) ((a) * (1.0f - (t)) + (b) * (t))

typedef struct {
  float x;
  float y;
} vector_t;

typedef struct {
  float x;
  float y;

  float vx;
  float vy;

  float size;//pensado para ser un radio, se puede usar para suplantar masa

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
  float3 v[TERRAIN_SIZE_X + 1][TERRAIN_SIZE_Y + 1];
  float3 normals[TERRAIN_SIZE_X + 1][TERRAIN_SIZE_Y + 1];
  vector_t slopes[TERRAIN_SIZE_X][TERRAIN_SIZE_Y];

  uint32_t indices[TERRAIN_SIZE_X * TERRAIN_SIZE_Y * 6];
} terrain_t;

typedef struct {
  float x;
  float y;

  int size;
} hole_t;

typedef enum {
  GG_SCREEN_TITLE,
  GG_SCREEN_PLAYERSELECT,
  GG_SCREEN_GAME,
} gg_screen_t;

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

uint32_t pc_base, pc_face, pc_club;

uint64_t frametime = 0;
uint64_t totalTicks = 0;

static inline void updateTimer() {
  frametime = _syscall(SYS_TICKS) - totalTicks;
  totalTicks += frametime;
}

void drawTerrainDebug(terrain_t *terrain) {
  uint32_t w = VGA_WIDTH / TERRAIN_SIZE_X;
  uint32_t h = VGA_HEIGHT / TERRAIN_SIZE_Y;

  for (uint32_t y = 0; y < TERRAIN_SIZE_Y; y++) {
    for (uint32_t x = 0; x < TERRAIN_SIZE_X; x++) {
      vector_t s = terrain->slopes[x][y];

      color_t color = rgba(
        (uint32_t) (s.x * 127 + 127), (uint32_t) (s.y * 127 + 127), 127, 0
      );
      vga_rect(x * w, y * h, (x + 1) * w - 1, (y + 1) * h - 1, color, 0);
    }
  }
  for (uint32_t y = 0; y <= TERRAIN_SIZE_Y; y++) {
    for (uint32_t x = 0; x <= TERRAIN_SIZE_X; x++) {
      float3 n = terrain->normals[x][y];
      vga_line(w * x, h * y, w * x + n.x * 20, h * y + n.z * 20, 0x0, 0);
    }
  }
}

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

void drawEnviroment(enviroment_t *env) {
  vga_rect(
    (env->x - env->size), (env->y - env->size), (env->x + env->size),
    (env->y + env->size), 0xff009000, 0
  );
}

void drawHole(hole_t *hole) {
  vga_rect(
    (hole->x - hole->size), (hole->y - hole->size), (hole->x + hole->size),
    (hole->y + hole->size), 0xff000000, 0
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
*/
void updatePlayerTank(physicsObject_t *obj, keycode_t keys[4]) {
  int up = kbd_keydown(keys[0]);
  int down = kbd_keydown(keys[1]);
  int right = kbd_keydown(keys[2]);
  int left = kbd_keydown(keys[3]);

  if (right) {
    obj->angle += TURNS_SPEED * frametime;
    if (obj->angle > M_PI) obj->angle -= 2.0f * M_PI;
  }
  if (left) {
    obj->angle -= TURNS_SPEED * frametime;
    if (obj->angle < -M_PI) obj->angle += 2.0f * M_PI;
  }
  if (up) {
    vector_t dir;
    // TODO make acceleration a constant
    dir.x = ACCELERATION * frametime * cos(obj->angle);
    dir.y = ACCELERATION * frametime * sin(obj->angle);
    accelerateObject(obj, &dir);
  }
  if (down) {
    obj->vx *= BRAKING;
    obj->vy *= BRAKING;
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
  obj->vx *= 1.0f - obj->drag * frametime;
  obj->vy *= 1.0f - obj->drag * frametime;

  //update pos
  obj->x += obj->vx;
  obj->y += obj->vy;

  // //che maxBounds
  if ((obj->x - obj->size) < 0.0f || (obj->x + obj->size) > FIELD_WIDTH) {
    obj->x = oldx;
    obj->vx = -(obj->vx);
  }
  if ((obj->y - obj->size) < 0.0f || (obj->y + obj->size) > FIELD_HEIGHT) {
    obj->y = oldy;
    obj->vy = -(obj->vy);
  }
}

/*
* asumiendo que son circulos 
* retorna el vetor de 'a' a 'b'
*/ //notalolo: final menos inicial
int checkCollision(physicsObject_t *a, physicsObject_t *b, vector_t *dir) {
  float difx = b->x - a->x;
  float dify = b->y - a->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(b->size + a->size)) {
    //TODO, habria que normalizarlo o algo asi/ ///////////////////////
    dir->x = (difx);
    dir->y = (dify);
    return 1;
  } else {
    return 0;
  }
}

/*
* checks if a colition happens and applyes a repeling vel
*/
void doCollision(physicsObject_t *a, physicsObject_t *b) {
  float va = abs(a->vx) + abs(a->vy);
  float vb = abs(b->vx) + abs(b->vy);

  vector_t dir = {0};
  if (checkCollision(a, b, &dir)) {
    vector_t dirb = dir;
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
int checkEnviroment(enviroment_t *env, physicsObject_t *obj, vector_t *dir) {
  float difx = env->x - obj->x;
  float dify = env->y - obj->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(env->size + obj->size)) {
    dir->x = sign(difx);
    dir->y = sign(dify);
    // en vez de signo habria que normalizarlo para q el modulo sea siempre 1 o algo asi
    return 1;
  } else {
    return 0;
  }
}

/*
* checks if obj is in a hole or mount and applyes a apropiate vel
*/
void applyGravity(terrain_t *terrain, physicsObject_t *obj) {
  uint32_t x = obj->x / TERRAIN_SIZE_UNITS_X;
  uint32_t y = obj->y / TERRAIN_SIZE_UNITS_Y;

  x = max(0, min(TERRAIN_SIZE_X - 1, x));
  y = max(0, min(TERRAIN_SIZE_Y - 1, y));

  vector_t s = terrain->slopes[x][y];

  obj->vx -= GRAVITY * s.x * abs(s.x);
  obj->vy -= GRAVITY * s.y * abs(s.y);
}

/*
*   valida si la pelota entra en el agujero y gana
*/
int checkHole(physicsObject_t *obj, hole_t *hole) {
  float difx = hole->x - obj->x;
  float dify = hole->y - obj->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(hole->size + obj->size)) {
    return 1;
  } else {
    return 0;
  }
}

static void generateTerrain(terrain_t *terrain) {
  // Generate terrain vertices
  for (int y = 0; y <= TERRAIN_SIZE_Y; y++) {
    for (int x = 0; x <= TERRAIN_SIZE_X; x++) {
      // test terrain gen
      float height = sqr((float) y / TERRAIN_SIZE_Y * 2.0f - 1.0f) * 2.0f +
                     sqr((float) x / TERRAIN_SIZE_X * 2.0f - 1.0f) * 2.0f;

      float3 vertex = {
        (float) x * TERRAIN_SIZE_UNITS_X - 0.5f * FIELD_WIDTH, height,
        (float) y * TERRAIN_SIZE_UNITS_Y - 0.5f * FIELD_HEIGHT
      };
      terrain->v[x][y] = vertex;
    }
  }

  // Generate terrain normals from heightmap for rendering
  for (int y = 0; y <= TERRAIN_SIZE_Y; y++) {
    for (int x = 0; x <= TERRAIN_SIZE_X; x++) {
      terrain->normals[x][y].y = 1.0f;

      int x0 = max(0, x - 1);
      int x1 = min(TERRAIN_SIZE_X, x + 1);
      int y0 = max(0, y - 1);
      int y1 = min(TERRAIN_SIZE_Y, y + 1);

      float h[] = {
        height(terrain, x0, y0),
        height(terrain, x1, y0),
        height(terrain, x0, y1),
        height(terrain, x1, y1),
      };
      float dx = ((h[1] + h[2]) - (h[0] + h[2]));
      float dy = ((h[2] + h[3]) - (h[0] + h[1]));

      float sx = dx / TERRAIN_SIZE_UNITS_X;
      float sy = dy / TERRAIN_SIZE_UNITS_Y;

      float3 normal = {sx, 1.0f, sy};
      terrain->normals[x][y] = vnorm(normal);
    }
  }

  // Generate terrain slopes for physics
  for (uint32_t y = 0; y < TERRAIN_SIZE_Y; y++) {
    for (uint32_t x = 0; x < TERRAIN_SIZE_X; x++) {
      float h[] = {
        height(terrain, x, y),
        height(terrain, x + 1, y),
        height(terrain, x, y + 1),
        height(terrain, x + 1, y + 1),
      };
      float dx = ((h[1] + h[2]) - (h[0] + h[2]));
      float dy = ((h[2] + h[3]) - (h[0] + h[1]));

      float sx = dx / TERRAIN_SIZE_UNITS_X;
      float sy = dy / TERRAIN_SIZE_UNITS_Y;

      terrain->slopes[x][y].x = sx;
      terrain->slopes[x][y].y = sy;
    }
  }

  // Generate terrain indices for rendering
  for (uint32_t y = 0; y < TERRAIN_SIZE_Y; y++) {
    for (uint32_t x = 0; x < TERRAIN_SIZE_X; x++) {
      uint32_t i = y * TERRAIN_SIZE_X + x;
      uint32_t iv = x * (TERRAIN_SIZE_Y + 1) + y;

      terrain->indices[i * 6 + 0] = iv;
      terrain->indices[i * 6 + 1] = iv + (TERRAIN_SIZE_Y + 1);
      terrain->indices[i * 6 + 2] = iv + (TERRAIN_SIZE_Y + 1) + 1;
      terrain->indices[i * 6 + 3] = iv;
      terrain->indices[i * 6 + 4] = iv + (TERRAIN_SIZE_Y + 1) + 1;
      terrain->indices[i * 6 + 5] = iv + 1;
    }
  }
}

static void setupGameRender() {
  // Set half render resolution for the game for increased framerate
  gfx_setRenderResolution(GFX_RES_HALF);

  // Set up view and projection matrices
  float3 pos = {0, 10.0f, 3.5f};
  float3 target = {0, 0, 0};
  float3 up = {0, 1, 0};

  float4x4 view = mat_lookat(pos, target, up);
  gfx_setMatrix(GFX_MAT_VIEW, &view);

  float fovDegrees = 75.0f;
  float4x4 projection =
    mat_perspective(deg2rad(fovDegrees), 4.0f / 3.0f, 0.1f, 100.0f);
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
}

static void renderTerrain(terrain_t *terrain) {
  // Set transform to identity
  float4x4 model = mat_scale(1, 1, 1);
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  float3 terrainColor = {0.1, 0.3, 0.2};

  // Draw the terrain
  gfx_drawPrimitivesIndexed(
    (float3 *) terrain->v, (float3 *) terrain->normals, terrain->indices,
    terrain->indices, TERRAIN_SIZE_X * TERRAIN_SIZE_Y * 2, terrainColor
  );
  // gfx_drawWireframeIndexed(
  //   (float3 *) terrain->v, terrain->indices,
  //   TERRAIN_SIZE_X * TERRAIN_SIZE_Y * 2, terrainColor
  // );
}

static void renderPlayer(physicsObject_t *player, terrain_t *terrain) {
  float angle = -player->angle - M_PI * 0.5f;
  if (angle < -M_PI) angle += M_PI * 2.0f;

  float fx = player->x / TERRAIN_SIZE_UNITS_X;
  float fy = player->y / TERRAIN_SIZE_UNITS_Y;
  uint32_t x = fx;
  uint32_t y = fy;

  x = max(0, min(TERRAIN_SIZE_X - 1, x));
  y = max(0, min(TERRAIN_SIZE_Y - 1, y));

  float h[] = {
    height(terrain, x, y),
    height(terrain, x + 1, y),
    height(terrain, x, y + 1),
    height(terrain, x + 1, y + 1),
  };

  float h0 = lerp(h[0], h[1], fx - x);
  float h1 = lerp(h[2], h[3], fx - x);

  float height = lerp(h0, h1, fy - y);

  float4x4 model = mat_rotationY(angle);
  model = mmul(
    mat_translation(
      player->x - FIELD_WIDTH * 0.5f, height, player->y - FIELD_HEIGHT * 0.5f
    ),
    model
  );
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  float3 color_base = {0.232f, 0.115f, 0.087f};
  float3 color_face = {0.082f, 0.021f, 0.001f};
  float3 color_club = {0.706f, 0.706f, 0.706f};

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
}

/*
 * Show the title screen/main menu and handle input.
 * Returns when the game should be started.
 */
static void showTitleScreen() {
  gg_screen_t screen = GG_SCREEN_TITLE;
  float a = 0.0f, capyAngle = (M_PI * -0.75);
  uint32_t textBlinkTimer = 0;

  /*
   * Graphics setup
   */

  // Set full render resolution for the main menu, we're not rendering much
  gfx_setRenderResolution(GFX_RES_FULL);

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
  while (screen != GG_SCREEN_GAME) {
    // Update the timer
    updateTimer();

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

    // Draw UI text
    if (screen == GG_SCREEN_TITLE &&
        textBlinkTimer > (TITLE_TEXT_BLINK_MS >> 1)) {
      vga_text(424, 500, "Press any key to start", 0xffffff, 0, VGA_TEXT_BG);
    }

    if (screen == GG_SCREEN_PLAYERSELECT) {
      vga_rect(300, 400, 700, 600, 0, 0);
      vga_text(320, 420, "TODO player select", 0xffffff, 0, 0);
      vga_text(320, 440, "Press RETURN to play", 0xffffff, 0, 0);

      // TODO: player select screen (1P - 2P)
    }

    vga_text(824, 736, "(c) 1998 TONKATSU GAMES", 0xffffff, 0, 0);

    // Draw everything to screen
    vga_present();

    // Update vars
    // TODO deltatime
    a += 0.005f * frametime;
    if (a > M_PI) a -= 2.0f * M_PI;
    capyAngle -= 0.01f * frametime;
    if (capyAngle < -M_PI) capyAngle += 2.0f * M_PI;
    textBlinkTimer = (textBlinkTimer + frametime) % TITLE_TEXT_BLINK_MS;

    // Process input
    ev = kbd_getKeyEvent();
    if (ev.key) {
      if (screen == GG_SCREEN_TITLE) {
        screen = GG_SCREEN_PLAYERSELECT;
      } else if (screen == GG_SCREEN_PLAYERSELECT && ev.key == KEY_RETURN) {
        screen = GG_SCREEN_GAME;
      }
    }
  }
}

/*
* Setup y main game loop
*/
int gg_startGame() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  // Init deltatime timer
  totalTicks = _syscall(SYS_TICKS);

  // Display the title screen
  showTitleScreen();

  /*
   * Set up game objects
   */
  terrain_t terrain;
  generateTerrain(&terrain);

  physicsObject_t p1 = {0};
  p1.color = 0xFF0000ff;
  p1.x = FIELD_WIDTH * 0.5f;
  p1.y = FIELD_HEIGHT * 0.5f;
  p1.drag = 0.06f;
  p1.size = 0.05f;
  p1.mass = 0.1f;
  p1.angle = 0.0f;

  physicsObject_t p2 = {0};
  p2.color = 0xFFff0000;
  p2.x = FIELD_WIDTH * 0.5f;
  p2.y = FIELD_HEIGHT * 0.5f;
  p2.drag = 0.06f;
  p2.size = 0.05f;
  p2.mass = 0.1f;
  p2.angle = 0.0f;

  physicsObject_t ball = {0};
  ball.color = 0xffb0b0b0;
  ball.x = VGA_WIDTH * 0.5f;  // 4.0f;
  ball.y = VGA_HEIGHT * 0.25f;//* 4.0f;
  ball.drag = 0.005f;
  ball.size = 0.02f;
  ball.mass = 1;

  // hole_t winingHole = {0};
  // winingHole.x = 100;
  // winingHole.y = 100;
  // winingHole.size = 50;

  keycode_t p1Keys[] = {KEY_W, KEY_S, KEY_D, KEY_A};
  keycode_t p2Keys[] = {
    KEY_ARROW_UP, KEY_ARROW_DOWN, KEY_ARROW_RIGHT, KEY_ARROW_LEFT
  };

  // Setup game graphics
  setupGameRender();

  /*
   * Game loop
   */
  int loop = 1;
  while (loop) {
    // Update the timer
    updateTimer();

    // Update keyboard input
    kbd_pollEvents();

    // Update physics
    updatePlayerTank(&p1, p1Keys);
    updateObject(&p1);
    doCollision(&p1, &ball);

    updatePlayerTank(&p2, p2Keys);
    updateObject(&p2);
    doCollision(&p2, &ball);

    updateObject(&ball);

    doCollision(&p1, &p2);

    applyGravity(&terrain, &ball);
    // if (checkHole(&ball, &winingHole)) { loop = 0; }

    // Draw game
    // vga_clear(0xFF00FF00);
    // drawTerrainDebug(&terrain);
    // drawEnviroment(&env);
    // drawObject(&p2);
    // drawObject(&p1);
    // drawObject(&ball);
    // drawHole(&winingHole);

    gfx_clear(0);

    renderPlayer(&p1, &terrain);
    renderPlayer(&p2, &terrain);

    renderTerrain(&terrain);

    gfx_present();
    vga_present();
  }

  // Restore status bar enabled state
  _syscall(SYS_STATUS_SET_ENABLED, statusEnabled);

  return 0;
}
