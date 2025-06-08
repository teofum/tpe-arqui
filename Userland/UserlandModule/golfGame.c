#include <fpmath.h>
#include <golfGame.h>
#include <graphics.h>
#include <kbd.h>
#include <print.h>
#include <rng.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include <syscall.h>
#include <vga.h>

#define deg2rad(x) ((x) / 180.0f * M_PI)

#define TITLE_TEXT_BLINK_MS 400

#define FIELD_WIDTH 15.0f
#define FIELD_HEIGHT 10.0f

#define TERRAIN_SIZE_X 15
#define TERRAIN_SIZE_Y 10
#define TERRAIN_SIZE_UNITS_X (FIELD_WIDTH / TERRAIN_SIZE_X)
#define TERRAIN_SIZE_UNITS_Y (FIELD_HEIGHT / TERRAIN_SIZE_Y)

// Terarin gen parameters
#define TERRAIN_NOISE_MAX 0.5f
#define TERRAIN_N_WAVES 6
#define TERRAIN_HILL_HEIGHT 0.75f
#define TERRAIN_HILL_WIDTH 2.0f

#define MAX_PLAYERS 2

// Multiplies all velocities
#define VMUL 0.1f

#define VMAX 9.0f
#define TURNS_SPEED 0.008f
#define ACCELERATION 0.005f
#define GRAVITY 0.1f
#define BRAKING 0.9

// Hit debounce so it doesn't register multiple times in a row
#define HIT_DEBOUNCE_MS 100

// Number of holes per game
#define DEFAULT_HOLES 3
#define MAX_HOLES 9

// Par is fixed for all levels
#define PAR 4

// Win/lose animation speed
#define ANIM_SPEED 0.02f

// UI colors and sizes
#define UI_GREEN_DARK 0xff002800
#define UI_GREEN_LIGHT 0xff005000
#define UI_GREEN_HIGHLIGHT 0xff008000

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define sign(x) (((x) == 0) ? 0 : ((x) > 0 ? 1 : -1))
#define sqr(x) ((x) * (x))

#define rgba(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define height(t, xx, yy) (t->v[(xx)][(yy)].y)

#define lerp(a, b, t) ((a) * (1.0f - (t)) + (b) * (t))
#define vlerp(a, b, t) vadd(vmuls((a), 1.0f - (t)), vmuls((b), (t)))

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
} physicsObject_t;

typedef struct {
  float3 v[TERRAIN_SIZE_X + 1][TERRAIN_SIZE_Y + 1];
  float3 normals[TERRAIN_SIZE_X + 1][TERRAIN_SIZE_Y + 1];

  uint32_t indices[TERRAIN_SIZE_X * TERRAIN_SIZE_Y * 6];
} terrain_t;

typedef struct {
  float x;
  float y;

  float size;
} hole_t;

typedef struct {
  uint32_t nPlayers;
  uint32_t nHoles;

  uint32_t scores[MAX_PLAYERS][MAX_HOLES + 1];
} gameSettings_t;

typedef enum {
  GG_SCREEN_TITLE,
  GG_SCREEN_PLAYERSELECT,
  GG_SCREEN_SETUP,
} gg_screen_t;

typedef enum {
  GG_MM_SINGLEPLAYER = 0,
  GG_MM_MULTIPLAYER,
  GG_MM_QUIT,
  GG_SM_HOLESELECT = 0,
  GG_SM_START,
  GG_SM_BACK,
} gg_menuOption_t;

typedef enum {
  GG_PAUSE_RESUME,
  GG_PAUSE_SKIP,
  GG_PAUSE_QUIT,
} gg_pauseOption_t;

typedef enum {
  GG_GAME_RUNNING,
  GG_GAME_END,
  GG_GAME_SCOREBOARD,
} gg_gameState_t;

typedef enum {
  GG_ANIM_NONE,
  GG_ANIM_WIN,
  GG_ANIM_LOSE,
} gg_playerAnim_t;

/*
 * Background framebuffers for optimized pre-rendering
 */
static uint8_t bgFramebuffer[VGA_WIDTH * VGA_HEIGHT * 3];
static float bgDepthbuffer[VGA_WIDTH * VGA_HEIGHT];

/*
 * Bitmap image data
 */
static uint8_t *titlescreenLogo = (uint8_t *) 0x3000000;

/*
 * OBJ strings for 3D models
 */
extern const char *obj_capybase;
extern const char *obj_capyface;
extern const char *obj_capyclub;
extern const char *obj_flag;
extern const char *obj_flagpole;
extern const char *obj_ball;

/*
 * 3D model data
 */
static float3 v_base[150];
static float3 n_base[150];
static uint32_t vi_base[280 * 3];
static uint32_t ni_base[280 * 3];

static float3 v_face[23];
static float3 n_face[23];
static uint32_t vi_face[20 * 3];
static uint32_t ni_face[20 * 3];

static float3 v_club[12];
static float3 n_club[12];
static uint32_t vi_club[20 * 3];
static uint32_t ni_club[20 * 3];

static float3 v_flag[5];
static float3 n_flag[4];
static uint32_t vi_flag[4 * 3];
static uint32_t ni_flag[4 * 3];

static float3 v_pole[8];
static float3 n_pole[8];
static uint32_t vi_pole[9 * 3];
static uint32_t ni_pole[9 * 3];

static float3 v_ball[12];
static float3 n_ball[12];
static uint32_t vi_ball[20 * 3];
static uint32_t ni_ball[20 * 3];

static uint32_t pc_base, pc_face, pc_club, pc_flag, pc_pole, pc_ball;

static float3 v_hole[7];
static uint32_t vi_hole[15] = {1, 6, 0, 1, 2, 3, 6, 4, 5, 6, 3, 4, 1, 3, 6};

static float3 v_walls[] = {
  {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1, 1},
  {1, -1, -1},  {1, -1, 1},  {1, 1, -1},  {1, 1, 1},
};
static uint32_t vi_walls[] = {
  0, 1, 2, 2, 1, 3, 4, 6, 5, 6, 7, 5, 0, 2, 4, 2, 6, 4, 1, 5, 3, 3, 5, 7,
};
static float3 n_walls[] = {
  {1, 0, 0},
  {-1, 0, 0},
  {0, 0, 1},
  {0, 0, -1},
};
static uint32_t ni_walls[] = {
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
};
static uint32_t pc_walls = sizeof(vi_walls) / sizeof(uint32_t);

/*
 * Frametime counter
 */
static uint64_t frametime = 0;
static uint64_t totalTicks = 0;

/*
 * Constant colors
 */
static float3 color_base[] = {
  {0.232f, 0.115f, 0.087f},
  {0.172f, 0.105f, 0.147f},
};
static float3 color_face = {0.082f, 0.021f, 0.001f};
static float3 color_club = {0.706f, 0.706f, 0.706f};
static float3 color_ball[] = {
  {0.8f, 0.8f, 0.8f},
  {0.8f, 0.4f, 0.25f},
  {0.25f, 0.4f, 0.8f},
};

/*
 * Score strings
 */
static const char *scoreNames[] = {"Hole in ZERO!", "Hole in One!", "Eagle",
                                   "Birdie",        "Par",          "Bogey",
                                   "Double Bogey",  "over Par"};
static const char *scoreTexts[] = {
  "How??",        "YOU'RE WINNER",      "Great!",
  "Nicely done!", "Not bad, not great", "Almost there...",
  "git gud",      "Major Skill Issue",
};

/*
 * Helper functions
 */
static inline void updateTimer() {
  frametime = _syscall(SYS_TICKS) - totalTicks;
  totalTicks += frametime;
}

static inline void makeHoleMesh() {
  for (int i = 0; i < 7; i++) {
    float a = -M_PI + 2.0f * i / 7.0f * M_PI;
    float3 v = {cos(a), 0.0f, sin(a)};
    v_hole[i] = v;
  }
}

static inline float randomFloat(pcg32_random_t *rng) {
  // Convert a random integer to a float in the 0-1 range
  return (pcg32_rand(rng) >> 8) * 0x1p-24f;// funky hex float notation
}

/*
* Aplica "aceleracion" y actualiza el estado 
*/
static void accelerateObject(physicsObject_t *obj, vector_t *dir) {
  // Add velocity
  obj->vx += dir->x;
  obj->vy += dir->y;

  // Cap velocity
  // TODO this should be a parameter for different objects
  float v = sqrt(sqr(obj->vx) + sqr(obj->vy));
  float vmax = VMAX / frametime;
  if (v > vmax) {
    float factor = vmax / v;
    obj->vx *= factor;
    obj->vy *= factor;
  }
}

/*
* tank controls for player, accelerates it
*/
static void updatePlayerTank(physicsObject_t *obj, keycode_t keys[4]) {
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
static void updateObject(physicsObject_t *obj) {
  float oldvx = obj->vx;// es para que no oscile dont worry about it
  float oldvy = obj->vy;

  float oldx = obj->x;//para bounds
  float oldy = obj->y;

  //add drag
  float dragMlt = 1.0f - obj->drag * 0.001f;
  dragMlt = max(0.0f, dragMlt);

  // ok this is stupid BUT
  for (uint32_t i = 0; i < frametime; i++) {
    obj->vx *= dragMlt;
    obj->vy *= dragMlt;
  }

  //update pos
  obj->x += obj->vx * VMUL * frametime;
  obj->y += obj->vy * VMUL * frametime;

  // check maxBounds
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
static inline int
checkCollision(physicsObject_t *a, physicsObject_t *b, vector_t *dir) {
  float difx = b->x - a->x;
  float dify = b->y - a->y;
  float dist = sqrt(sqr(difx) + sqr(dify));

  if (dist <= b->size + a->size) {
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
static int doCollision(physicsObject_t *a, physicsObject_t *b) {
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

    return 1;
  }

  return 0;
}

/*
* checks if obj is in a hole or mount and applies a apropiate vel
*/
static void applyGravity(terrain_t *terrain, physicsObject_t *obj) {
  float fx = obj->x / TERRAIN_SIZE_UNITS_X;
  float fy = obj->y / TERRAIN_SIZE_UNITS_Y;

  uint32_t x = fx, y = fy;
  x = max(0, min(TERRAIN_SIZE_X - 1, x));
  y = max(0, min(TERRAIN_SIZE_Y - 1, y));

  float3 normals[] = {
    terrain->normals[x][y],
    terrain->normals[x + 1][y],
    terrain->normals[x][y + 1],
    terrain->normals[x + 1][y + 1],
  };

  float3 normal = vlerp(
    vlerp(normals[0], normals[1], fx - x),
    vlerp(normals[2], normals[3], fx - x), fy - y
  );
  vector_t s = {normal.x, normal.z};

  vector_t a = {
    -GRAVITY * s.x * abs(s.x),
    -GRAVITY * s.y * abs(s.y),
  };
  accelerateObject(obj, &a);
}

/*
*   valida si la pelota entra en el agujero y gana
*/
static int checkHole(physicsObject_t *obj, hole_t *hole) {
  float difx = hole->x - obj->x;
  float dify = hole->y - obj->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(hole->size + obj->size)) {
    return 1;
  } else {
    return 0;
  }
}


float doHill(int curx, int cury, int x, int y, float size) {
  float difx = x - curx;
  float dify = y - cury;
  float dist = sqrt(sqr(difx) + sqr(dify));

  if (dist <= size) {
    return (1 - dist / size) * TERRAIN_HILL_HEIGHT;
  } else {
    return 0;
  }
}

static void generateTerrain(terrain_t *terrain, pcg32_random_t *rng) {
  // Generate terrain vertices

  // frequency/ numero de armonico
  float fy[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    fy[i] = ((float) (pcg32_rand(rng) % 10)) / 10;
  }
  float fx[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    fx[i] = ((float) (pcg32_rand(rng) % 10)) / 10;
  }

  // fase, offset
  float offsety[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    offsety[i] = ((float) (pcg32_rand(rng) % 5) / 5) * 2 * M_PI;
  }
  float offsetx[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    offsetx[i] = ((float) (pcg32_rand(rng) % 5) / 5) * 2 * M_PI;
  }

  // hill position
  int hillx = (pcg32_rand(rng) % TERRAIN_SIZE_X);
  int hilly = (pcg32_rand(rng) % TERRAIN_SIZE_Y);

  // pit position
  int pitx = (pcg32_rand(rng) % TERRAIN_SIZE_X);
  int pity = (pcg32_rand(rng) % TERRAIN_SIZE_Y);

  // loop
  for (int y = 0; y <= TERRAIN_SIZE_Y; y++) {
    for (int x = 0; x <= TERRAIN_SIZE_X; x++) {
      // test terrain gen ///////////////////////////////////////lolo tocar//
      float height = 0;
      for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
        height +=
          ((sin(fitToPi((x * TERRAIN_SIZE_UNITS_X) * fy[i] + offsety[i])) *
            TERRAIN_NOISE_MAX) +
           (sin(fitToPi((y * TERRAIN_SIZE_UNITS_X) * fx[i] + offsetx[i])) *
            TERRAIN_NOISE_MAX)) /
          TERRAIN_N_WAVES;
      }

      // hill
      height += doHill(x, y, hillx, hilly, TERRAIN_HILL_WIDTH);

      // pit
      height -= doHill(x, y, pitx, pity, TERRAIN_HILL_WIDTH);

      // todos los bordes levantados
      if ((x == FIELD_WIDTH || x == 0) || (y == FIELD_HEIGHT || y == 0)) {
        height = max(height, 0.0f) + TERRAIN_NOISE_MAX / 2;
      }

      // // solo las esquinas levantadas
      // if ((x == FIELD_WIDTH || x == 0) && (y == FIELD_HEIGHT || y == 0)) {
      //   height += TERRAIN_NOISE_MAX / 4;
      // }

      /////////////////////////
      // ideas:
      // hills y pozos
      // algun ruido con trigs
      //
      ///////////////////////////////////////////////////////////////////////
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

static void setupGameRender(float4x4 *view) {
  // Set half render resolution for the game for increased framerate
  gfx_setFlag(GFX_HALFRES, 1);

  // Set up view and projection matrices
  gfx_setMatrix(GFX_MAT_VIEW, view);

  float fovDegrees = 75.0f;
  float4x4 projection =
    mat_perspective(deg2rad(fovDegrees), 4.0f / 3.0f, 0.1f, 100.0f);
  gfx_setMatrix(GFX_MAT_PROJECTION, &projection);

  // Set up lighting
  float3 light = {-2, 3, -2};
  float3 lightcolor = {1.80f, 1.75f, 1.60f};
  float3 ambient = vmuls(lightcolor, 0.5f);

  gfx_light_t lightType = GFX_LIGHT_DIRECTIONAL;
  gfx_setLightType(lightType);
  gfx_setLight(GFX_LIGHT_POSITION, &light);
  gfx_setLight(GFX_AMBIENT_LIGHT, &ambient);
  gfx_setLight(GFX_LIGHT_COLOR, &lightcolor);
}

static void renderTerrain(terrain_t *terrain) {
  // Draw background
  vga_setFramebuffer(gfx_getFramebuffer());
  vga_gradient(
    0, 0, (VGA_WIDTH >> 1) - 1, (VGA_HEIGHT >> 1) - 1,
    colors(0x1a32e6, 0x07d0f8), VGA_GRAD_V
  );
  vga_shade(
    0, 0, (VGA_WIDTH >> 1) - 1, (VGA_HEIGHT >> 1) - 1, 0x50000080,
    VGA_ALPHA_BLEND
  );
  vga_setFramebuffer(NULL);

  // Set transform to identity
  float4x4 model = mat_scale(1, 1, 1);
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  float3 terrainColor = {0.18, 0.37, 0.05};

  // Draw the terrain
  gfx_drawPrimitivesIndexed(
    (float3 *) terrain->v, (float3 *) terrain->normals, terrain->indices,
    terrain->indices, TERRAIN_SIZE_X * TERRAIN_SIZE_Y * 2, terrainColor
  );

  // Draw terrain bound walls
  float hmax = (TERRAIN_HILL_HEIGHT + TERRAIN_NOISE_MAX + 0.5f);
  model = mat_scale(FIELD_WIDTH * 0.5f, hmax * 0.5f, FIELD_HEIGHT * 0.5f);
  model = mmul(mat_translation(0.0f, hmax * 0.25f, 0.0f), model);
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  float3 wallColor = {0.37, 0.18, 0.05};
  gfx_drawPrimitivesIndexed(
    v_walls, n_walls, vi_walls, ni_walls, pc_walls, wallColor
  );
}

static float getTerrainHeightAt(terrain_t *terrain, float fx, float fy) {
  fx /= TERRAIN_SIZE_UNITS_X;
  fx /= TERRAIN_SIZE_UNITS_Y;
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

  return lerp(h0, h1, fy - y);
}

static void renderPlayer(
  physicsObject_t *player, uint32_t i, gg_playerAnim_t anim, float t,
  terrain_t *terrain
) {
  float angle = -player->angle - M_PI * 0.5f;
  if (angle < -M_PI) angle += M_PI * 2.0f;

  float height = getTerrainHeightAt(terrain, player->x, player->y);

  float4x4 model = mat_rotationY(angle);

  if (anim == GG_ANIM_WIN) {
    t *= 0.1f;
    t = -M_PI + 2.0f * M_PI * (t - (uint32_t) t);
    model = mmul(model, mat_rotationY(t));
  } else if (anim == GG_ANIM_LOSE) {
    t *= 0.02f;
    t = max(min(t, 10.0f), 0.001f);
    t = sqrt(t);
    model =
      mmul(model, mat_scale(1.0f + t, 1.0f / (1.0f + t), 1.0f + 0.5f * t));
  }

  model = mmul(model, mat_scale(0.8f, 0.8f, 0.8f));
  model = mmul(
    mat_translation(
      player->x - FIELD_WIDTH * 0.5f, height, player->y - FIELD_HEIGHT * 0.5f
    ),
    model
  );
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  // Draw the capybara
  gfx_drawPrimitivesIndexed(
    v_face, n_face, vi_face, ni_face, pc_face, color_face
  );
  gfx_drawPrimitivesIndexed(
    v_base, n_base, vi_base, ni_base, pc_base, color_base[i]
  );
  gfx_drawPrimitivesIndexed(
    v_club, n_club, vi_club, ni_club, pc_club, color_club
  );
}

static void renderBall(physicsObject_t *ball, uint32_t i, terrain_t *terrain) {
  float height = getTerrainHeightAt(terrain, ball->x, ball->y);

  float4x4 model = mat_translation(
    ball->x - FIELD_WIDTH * 0.5f, height, ball->y - FIELD_HEIGHT * 0.5f
  );
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  gfx_drawPrimitivesIndexed(
    v_ball, n_ball, vi_ball, ni_ball, pc_ball, color_ball[i]
  );
}

static void renderHole(hole_t *hole, terrain_t *terrain) {
  float height = getTerrainHeightAt(terrain, hole->x, hole->y);
  float4x4 translation = mat_translation(
    hole->x - FIELD_WIDTH * 0.5f, height, hole->y - FIELD_HEIGHT * 0.5f
  );

  float4x4 model = mmul(translation, mat_scale(hole->size, 1.0f, hole->size));
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  float3 color_hole = {0.0f, 0.0f, 0.0f};
  gfx_setFlag(GFX_DEPTH_TEST | GFX_DEPTH_WRITE, 0);
  gfx_drawPrimitivesIndexed(v_hole, NULL, vi_hole, vi_hole, 5, color_hole);
  gfx_setFlag(GFX_DEPTH_TEST | GFX_DEPTH_WRITE, 1);

  model = mmul(translation, mat_rotationY(M_PI * 0.75f));
  gfx_setMatrix(GFX_MAT_MODEL, &model);

  float3 color_pole = {0.80f, 0.80f, 0.80f};
  float3 color_flag = {0.92f, 0.20f, 0.24f};
  gfx_drawPrimitivesIndexed(
    v_pole, n_pole, vi_pole, vi_pole, pc_pole, color_pole
  );
  gfx_drawPrimitivesIndexed(
    v_flag, n_flag, vi_flag, vi_flag, pc_flag, color_flag
  );
}

/*
 * Show the title screen/main menu and handle input.
 * Returns when the game should be started.
 */
static int showTitleScreen(gameSettings_t *settings) {
  gg_screen_t screen = GG_SCREEN_TITLE;
  float a = 0.0f, capyAngle = (M_PI * -0.75);
  uint32_t textBlinkTimer = 0;

  /*
   * Menu state
   */
  int menuItem = 0;
  static const char *menuStrings[] = {
    "Play (1 player)",
    "Play (2 player)",
    " Quit to Shell ",
  };
  static const char *setupStrings[] = {
    " Holes: X ",
    "Start Game",
    "   Back   ",
  };
  uint32_t menuItemCount = sizeof(menuStrings) / sizeof(menuStrings[0]);
  uint32_t setupItemCount = sizeof(setupStrings) / sizeof(setupStrings[0]);

  /*
   * Graphics setup
   */

  // Set full render resolution for the main menu, we're not rendering much
  gfx_setFlag(GFX_HALFRES, 0);

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

  /*
   * Draw loop
   */
  kbd_event_t ev = {0};
  while (1) {
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
    uint32_t nCapybaras = menuItem == GG_MM_MULTIPLAYER ? 2 : 1;
    if (screen == GG_SCREEN_SETUP) nCapybaras = settings->nPlayers;
    for (int i = 0; i < nCapybaras; i++) {
      float x = nCapybaras == 1 ? 0.0f : -1.5f + i * 3.0f;
      float4x4 model = mat_rotationY(capyAngle);
      model = mmul(mat_translation(x, -1.3f, 0), model);
      gfx_setMatrix(GFX_MAT_MODEL, &model);

      // Draw the capybara
      gfx_drawPrimitivesIndexed(
        v_face, n_face, vi_face, ni_face, pc_face, color_face
      );
      gfx_drawPrimitivesIndexed(
        v_base, n_base, vi_base, ni_base, pc_base, color_base[i]
      );
      gfx_drawPrimitivesIndexed(
        v_club, n_club, vi_club, ni_club, pc_club, color_club
      );
    }

    // Present gfx buffer to VGA main framebuffer
    gfx_present();

    // Restore the default VGA framebuffer to draw on top of the 3D graphics
    vga_setFramebuffer(NULL);

    // Draw the title logo (it floats!)
    vga_bitmap(256, 128 - 18 * sin(a), titlescreenLogo, 2, VGA_ALPHA_BLEND);

    // Draw UI text
    if (screen == GG_SCREEN_TITLE &&
        textBlinkTimer > (TITLE_TEXT_BLINK_MS >> 1)) {
      vga_font_t oldfont = vga_font(VGA_FONT_ALT_BOLD);
      vga_text(424, 500, "Press any key to start", 0xffffff, 0, VGA_TEXT_BG);
      vga_font(oldfont);
    }

    if (screen == GG_SCREEN_PLAYERSELECT) {
      vga_shade(384, 400, 639, 599, 0, 0);
      vga_rect(
        384, 400, 639, 599, 0xa0ffffff & UI_GREEN_LIGHT, VGA_ALPHA_BLEND
      );
      vga_frame(384, 400, 639, 599, 0xffffff, 0);

      vga_font_t oldfont = vga_font(VGA_FONT_LARGE);
      vga_text(458, 416, "MAIN MENU", 0xffffff, 0, 0);
      vga_font(oldfont);

      for (int i = 0; i < menuItemCount; i++) {
        if (i == menuItem) {
          vga_rect(
            428, 456 + 24 * i - 4, 595, 456 + 24 * i + 19, UI_GREEN_HIGHLIGHT, 0
          );
          vga_frame(428, 456 + 24 * i - 4, 595, 456 + 24 * i + 19, 0xffffff, 0);
        }
        vga_text(452, 456 + 24 * i, menuStrings[i], 0xffffff, 0, 0);
      }
    }

    if (screen == GG_SCREEN_SETUP) {
      vga_shade(384, 400, 639, 599, 0, 0);
      vga_rect(
        384, 400, 639, 599, 0xa0ffffff & UI_GREEN_LIGHT, VGA_ALPHA_BLEND
      );
      vga_frame(384, 400, 639, 599, 0xffffff, 0);

      vga_font_t oldfont = vga_font(VGA_FONT_LARGE);
      vga_text(452, 416, "GAME SETUP", 0xffffff, 0, 0);
      vga_font(oldfont);

      for (int i = 0; i < setupItemCount; i++) {
        if (i == menuItem) {
          vga_rect(
            428, 456 + 24 * i - 4, 595, 456 + 24 * i + 19, UI_GREEN_HIGHLIGHT, 0
          );
          vga_frame(428, 456 + 24 * i - 4, 595, 456 + 24 * i + 19, 0xffffff, 0);
        }
        if (i == 0) {
          char buf[20];
          sprintf(buf, " Holes: %u ", settings->nHoles);
          vga_text(472, 456, buf, 0xffffff, 0, 0);
        } else {
          vga_text(472, 456 + 24 * i, setupStrings[i], 0xffffff, 0, 0);
        }
      }
    }

    vga_text(824, 736, "(c) 1998 TONKATSU GAMES", 0xffffff, 0, 0);

    // Draw everything to screen
    vga_present();

    // Update vars
    a += 0.005f * frametime;
    if (a > M_PI) a -= 2.0f * M_PI;
    capyAngle -= 0.5f * ANIM_SPEED * frametime;
    if (capyAngle < -M_PI) capyAngle += 2.0f * M_PI;
    textBlinkTimer = (textBlinkTimer + frametime) % TITLE_TEXT_BLINK_MS;

    // Process input
    ev = kbd_getKeyEvent();
    if (ev.key) {
      if (screen == GG_SCREEN_TITLE) {
        screen = GG_SCREEN_PLAYERSELECT;
      } else if (screen == GG_SCREEN_PLAYERSELECT) {
        switch (ev.key) {
          case KEY_ARROW_UP:
            menuItem = menuItem == 0 ? menuItemCount - 1 : menuItem - 1;
            break;
          case KEY_ARROW_DOWN:
            menuItem = (menuItem + 1) % menuItemCount;
            break;
          case KEY_RETURN: {
            if (menuItem == GG_MM_QUIT) return 0;

            settings->nPlayers = menuItem + 1;
            menuItem = 0;
            screen = GG_SCREEN_SETUP;
          }
        }
      } else if (screen == GG_SCREEN_SETUP) {
        switch (ev.key) {
          case KEY_ARROW_UP:
            menuItem = menuItem == 0 ? setupItemCount - 1 : menuItem - 1;
            break;
          case KEY_ARROW_DOWN:
            menuItem = (menuItem + 1) % setupItemCount;
            break;
          case KEY_ARROW_LEFT:
            settings->nHoles = max(1, settings->nHoles - 1);
            break;
          case KEY_ARROW_RIGHT:
            settings->nHoles = min(MAX_HOLES, settings->nHoles + 1);
            break;
          case KEY_RETURN: {
            if (menuItem == GG_SM_BACK) {
              menuItem = 0;
              screen = GG_SCREEN_PLAYERSELECT;
            } else if (menuItem == GG_SM_START) {
              return 1;
            }
          }
        }
      }
    }
  }
}

static gg_pauseOption_t showPauseMenu() {
  int exit = 0;
  gg_pauseOption_t selected = 0;
  kbd_event_t ev;

  static const char *optionStrings[] = {
    "   Resume game   ",
    "    Skip hole    ",
    "Quit to main menu",
  };
  uint32_t optionCount = sizeof(optionStrings) / sizeof(optionStrings[0]);

  while (!exit) {
    updateTimer();

    uint16_t x0 = (VGA_WIDTH >> 1) - 128, x1 = x0 + 255;
    uint16_t y0 = (VGA_HEIGHT >> 1) - 96, y1 = y0 + 191;

    // Use current game graphics as background
    gfx_present();

    // Background
    vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_DARK, VGA_ALPHA_BLEND);
    vga_shade(x0, y0, x1, y1, UI_GREEN_DARK, 0);
    vga_frame(x0, y0, x1, y1, 0xffffff, 0);

    // Title
    vga_font_t oldfont = vga_font(VGA_FONT_LARGE);
    vga_text((VGA_WIDTH - 72) >> 1, y0 + 16, "PAUSED", 0xffffff, 0, 0);
    vga_font(oldfont);

    // Options
    for (int i = 0; i < optionCount; i++) {
      if (i == selected) {
        vga_rect(
          428, y0 + 48 + 24 * i - 4, 595, y0 + 48 + 24 * i + 19,
          UI_GREEN_HIGHLIGHT, 0
        );
        vga_frame(
          428, y0 + 48 + 24 * i - 4, 595, y0 + 48 + 24 * i + 19, 0xffffff, 0
        );
      }
      vga_text(
        (VGA_WIDTH - 136) >> 1, y0 + 48 + 24 * i, optionStrings[i], 0xffffff, 0,
        0
      );
    }

    vga_present();

    // Keyboard input
    ev = kbd_getKeyEvent();
    switch (ev.key) {
      case KEY_ARROW_UP:
        selected = selected == 0 ? optionCount - 1 : selected - 1;
        break;
      case KEY_ARROW_DOWN:
        selected = (selected + 1) % optionCount;
        break;
      case KEY_RETURN: {
        exit = 1;
      }
    }
  }

  return selected;
}

static int
playGame(gameSettings_t *settings, uint32_t nHole, pcg32_random_t *rng) {
  // Keymaps per player
  static keycode_t keys[2][4] = {
    {KEY_W, KEY_S, KEY_D, KEY_A},
    {KEY_ARROW_UP, KEY_ARROW_DOWN, KEY_ARROW_RIGHT, KEY_ARROW_LEFT}
  };

  gg_gameState_t gameState = GG_GAME_RUNNING;

  /*
   * Set up game objects
   */
  terrain_t terrain;
  generateTerrain(&terrain, rng);

  physicsObject_t players[MAX_PLAYERS];
  physicsObject_t balls[MAX_PLAYERS];
  int ballInPlay[MAX_PLAYERS];
  uint32_t hits[MAX_PLAYERS];
  uint32_t iframes[MAX_PLAYERS];// Hit counter debounce timer
  gg_playerAnim_t anim[MAX_PLAYERS];

  for (int i = 0; i < settings->nPlayers; i++) {
    players[i].x = FIELD_WIDTH * (randomFloat(rng) * 0.8f + 0.1f);
    players[i].y = FIELD_HEIGHT * (randomFloat(rng) * 0.8f + 0.1f);
    players[i].vx = 0.0f;
    players[i].vy = 0.0f;
    players[i].drag = 30.0f;
    players[i].size = 0.7f;
    players[i].mass = 0.1f;
    players[i].angle = 0.0f;

    balls[i].x = FIELD_WIDTH * (randomFloat(rng) * 0.8f + 0.1f);
    balls[i].y = FIELD_HEIGHT * (randomFloat(rng) * 0.8f + 0.1f);
    balls[i].vx = 0.0f;
    balls[i].vy = 0.0f;
    balls[i].drag = 12.0f;
    balls[i].size = 0.1f;
    balls[i].mass = 1.0f;

    hits[i] = 0;
    iframes[i] = 0;
    ballInPlay[i] = 1;
    anim[i] = GG_ANIM_NONE;
  }

  hole_t hole = {0};
  hole.x = FIELD_WIDTH * (randomFloat(rng) * 0.8f + 0.1f);
  hole.y = FIELD_HEIGHT * (randomFloat(rng) * 0.8f + 0.1f);
  hole.size = 0.5f;

  // Setup game graphics
  float3 viewPos = {0, 10.0f, 3.5f};
  float3 viewTarget = {0, 0, 0};
  float3 viewUp = {0, 1, 0};

  float4x4 view = mat_lookat(viewPos, viewTarget, viewUp);
  setupGameRender(&view);

  /*
   * Pre-render the terrain to a separate frame and depth buffer
   * Terrain rendering is by far the most expensive, and the camera doesn't
   * move during gameplay, so we can gain a huge perf boost by pre-rendering
   * terrain and reusing the frame and depth buffers.
   * This literally multiplies framerate five times!
   */
  gfx_setBuffers(bgFramebuffer, bgDepthbuffer);
  gfx_clear(0);
  renderTerrain(&terrain);
  gfx_setBuffers(NULL, NULL);

  /*
   * Game loop
   */
  int loop = 1;
  kbd_event_t ev = {0};
  float t = 0;// Timer for win/lose animation
  while (loop) {
    // Update the timer
    updateTimer();

    // Process input and physics only when the game is running
    if (gameState == GG_GAME_RUNNING) {
      // Update keyboard input
      kbd_pollEvents();

      /*
       * Physics and gameplay update
       */
      for (int i = 0; i < settings->nPlayers; i++) {
        // Apply inputs and update player physics
        updatePlayerTank(&players[i], keys[i]);
        updateObject(&players[i]);

        // Apply gravity and update ball physics
        if (ballInPlay[i]) {
          applyGravity(&terrain, &balls[i]);
          updateObject(&balls[i]);
        }

        // Collisions
        // Number of collisions scales with N^2, this is fine with just two players
        for (int j = 0; j < settings->nPlayers; j++) {
          // Player-ball collisions
          if (ballInPlay[j]) {
            int hit = doCollision(&players[i], &balls[j]);

            // Increase hit counter only when player hits their own ball
            if (hit && i == j && iframes[i] == 0) {
              hits[i]++;
              iframes[i] = HIT_DEBOUNCE_MS;
            }
          }

          // Player-player and ball-ball collisions
          if (i != j) {
            doCollision(&players[i], &players[j]);
            if (ballInPlay[i] && ballInPlay[j])
              doCollision(&balls[i], &balls[j]);
          }
        }

        // Check win condition
        if (ballInPlay[i] && checkHole(&balls[i], &hole)) {
          // Remove the ball from the playfield
          ballInPlay[i] = 0;

          // If no balls are left, end the game
          int endGame = 1;
          for (int j = 0; j < settings->nPlayers; j++) {
            if (ballInPlay[j]) endGame = 0;
          }

          if (endGame) {
            gameState = GG_GAME_END;

            // Calculate scores
            for (int j = 0; j < settings->nPlayers; j++) {
              settings->scores[j][nHole] =
                (hits[j] > PAR + 2) ? 0 : (PAR + 3 - hits[j]);
              settings->scores[j][MAX_HOLES] += settings->scores[j][nHole];
            }

            // Set game end player animations
            if (settings->nPlayers == 1) {
              anim[0] = hits[0] > PAR ? GG_ANIM_LOSE : GG_ANIM_WIN;
            } else if (settings->nPlayers == 2) {
              anim[0] = hits[0] > hits[1] ? GG_ANIM_LOSE : GG_ANIM_WIN;
              anim[1] = hits[1] > hits[0] ? GG_ANIM_LOSE : GG_ANIM_WIN;
            }

            break;
          }
        }

        // Update hit timers
        iframes[i] = iframes[i] <= frametime ? 0 : iframes[i] - frametime;
      }

      if (kbd_keypressed(KEY_ESCAPE)) {
        gg_pauseOption_t option = showPauseMenu();
        if (option == GG_PAUSE_SKIP) {
          gameState = GG_GAME_END;

          // Set scores to zero for this hole
          for (int j = 0; j < settings->nPlayers; j++) {
            settings->scores[j][nHole] = 0;
          }
          break;
        } else if (option == GG_PAUSE_QUIT) {
          return 1;
        }
      }
    }

    /*
     * Render graphics
     */
    if (gameState != GG_GAME_RUNNING) {
      // On endgame the camera *does* move, so we need to actually render the terrain
      gfx_clear(0);
      renderTerrain(&terrain);
    } else {
      // While playing, we can reuse the background we pre-rendered at the start
      gfx_copy(NULL, bgFramebuffer);
      gfx_depthcopy(NULL, bgDepthbuffer);
    }

    renderHole(&hole, &terrain);

    for (int i = 0; i < settings->nPlayers; i++) {
      renderPlayer(&players[i], i, anim[i], t, &terrain);
      if (ballInPlay[i])
        renderBall(&balls[i], settings->nPlayers == 1 ? 0 : i + 1, &terrain);
    }

    gfx_present();

    /*
     * Draw UI
     */
    char buf[64];

    // HUD background
    vga_gradient(
      0, 0, VGA_WIDTH - 1, 63, colors(UI_GREEN_LIGHT, UI_GREEN_DARK), 0
    );
    vga_shade(0, 0, VGA_WIDTH - 1, 63, UI_GREEN_DARK, 0);
    vga_line(0, 63, VGA_WIDTH - 1, 63, 0xffffff, 0);
    vga_line(VGA_WIDTH >> 1, 0, VGA_WIDTH >> 1, 63, 0xffffff, 0);

    vga_rect(
      (VGA_WIDTH >> 1) - 32, 12, (VGA_WIDTH >> 1) + 31, 51, UI_GREEN_DARK, 0
    );
    vga_frame(
      (VGA_WIDTH >> 1) - 32, 12, (VGA_WIDTH >> 1) + 31, 51, 0xffffff, 0
    );

    // Text
    vga_font_t oldfont = vga_font(VGA_FONT_ALT_BOLD);
    sprintf(buf, "Hole %u", nHole + 1);
    vga_text((VGA_WIDTH >> 1) - 24, 16, buf, 0xffffff, 0x000000, 0);
    vga_font(oldfont);
    sprintf(buf, "Par: %u", PAR);
    vga_text((VGA_WIDTH >> 1) - 24, 32, buf, 0xffffff, 0x000000, 0);

    vga_text(24, 16, "PLAYER 1", 0xffffff, 0x000000, 0);
    sprintf(buf, "Hits: %u", hits[0]);
    vga_text(24, 32, buf, 0xffffff, 0x000000, 0);

    vga_text(104, 16, " Hole", 0xffffff, 0x000000, 0);
    vga_text(104, 32, "Score", 0xffffff, 0x000000, 0);

    for (uint32_t h = 0; h < settings->nHoles; h++) {
      sprintf(buf, "%u", h + 1);
      vga_text(152 + h * 16, 16, buf, 0xffffff, 0, 0);
      sprintf(buf, "%u", settings->scores[0][h]);
      vga_text(152 + h * 16, 32, h >= nHole ? "-" : buf, 0xffffff, 0, 0);
    }

    if (settings->nPlayers > 1) {
      vga_text((VGA_WIDTH >> 1) + 56, 16, "PLAYER 2", 0xffffff, 0x000000, 0);
      sprintf(buf, "Hits: %u", hits[1]);
      vga_text((VGA_WIDTH >> 1) + 56, 32, buf, 0xffffff, 0x000000, 0);

      vga_text((VGA_WIDTH >> 1) + 136, 16, " Hole", 0xffffff, 0x000000, 0);
      vga_text((VGA_WIDTH >> 1) + 136, 32, "Score", 0xffffff, 0x000000, 0);

      for (uint32_t h = 0; h < settings->nHoles; h++) {
        sprintf(buf, "%u", h + 1);
        vga_text((VGA_WIDTH >> 1) + 194 + h * 16, 16, buf, 0xffffff, 0, 0);
        sprintf(buf, "%u", settings->scores[1][h]);
        vga_text(
          (VGA_WIDTH >> 1) + 194 + h * 16, 32, h >= nHole ? "-" : buf, 0xffffff,
          0, 0
        );
      }
    }

    /*
     * Display game end screen
     */
    if (gameState == GG_GAME_END) {
      uint16_t x0 = (VGA_WIDTH >> 1) - 128, x1 = x0 + 255;
      uint16_t y0 = (VGA_HEIGHT >> 1) - 96, y1 = y0 + 191;

      // Background
      vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_DARK, VGA_ALPHA_BLEND);
      vga_shade(x0, y0, x1, y1, UI_GREEN_DARK, 0);
      vga_frame(x0, y0, x1, y1, 0xffffff, 0);

      // Score counter (common to 1P-2P)
      uint16_t scoreX = (VGA_WIDTH - 184) >> 1;
      uint16_t scoreY = y0 + 96;
      vga_text(scoreX, scoreY, " Hole", 0xffffff, 0x000000, 0);

      for (uint32_t h = 0; h < settings->nHoles; h++) {
        sprintf(buf, "%u", h + 1);
        vga_text(scoreX + 48 + h * 16, scoreY, buf, 0xffffff, 0, 0);
        sprintf(buf, "%u", settings->scores[0][h]);
        vga_text(
          scoreX + 48 + h * 16, scoreY + 16, h > nHole ? "-" : buf, 0xffffff, 0,
          0
        );
      }

      if (settings->nPlayers == 1) {
        // Display score screen for singleplayer
        uint32_t score = min(hits[0], 7);
        if (score < 7) sprintf(buf, "%s", scoreNames[score]);
        else
          sprintf(buf, "%u %s", hits[0] - PAR, scoreNames[score]);
        uint32_t textWidth = strlen(buf) * 12;

        oldfont = vga_font(VGA_FONT_LARGE);
        vga_text((VGA_WIDTH - textWidth) >> 1, y0 + 16, buf, 0xffffff, 0, 0);
        vga_font(oldfont);

        sprintf(buf, "%s", scoreTexts[score]);
        textWidth = strlen(buf) * 8;
        vga_text((VGA_WIDTH - textWidth) >> 1, y0 + 64, buf, 0xffffff, 0, 0);

        // Single player score
        vga_text(scoreX, scoreY + 16, "Score", 0xffffff, 0x000000, 0);
      } else if (settings->nPlayers == 2) {
        // Display score screen for multiplayer
        if (hits[0] == hits[1]) {
          sprintf(buf, "     Draw     ");
        } else {
          sprintf(buf, "Player %u wins!", hits[0] < hits[1] ? 1 : 2);
        }

        oldfont = vga_font(VGA_FONT_LARGE);
        vga_text((VGA_WIDTH - 168) >> 1, y0 + 16, buf, 0xffffff, 0, 0);
        vga_font(oldfont);

        uint32_t score = min(min(hits[0], hits[1]), 7);
        if (score < 7) sprintf(buf, "%s", scoreNames[score]);
        else
          sprintf(buf, "%u %s", min(hits[0], hits[1]) - PAR, scoreNames[score]);
        uint32_t textWidth = strlen(buf) * 8;
        vga_text((VGA_WIDTH - textWidth) >> 1, y0 + 64, buf, 0xffffff, 0, 0);

        // Multiplayer scores
        vga_text(scoreX, scoreY + 16, "   P1", 0xffffff, 0x000000, 0);
        vga_text(scoreX, scoreY + 32, "   P2", 0xffffff, 0x000000, 0);

        for (uint32_t h = 0; h < settings->nHoles; h++) {
          sprintf(buf, "%u", settings->scores[1][h]);
          vga_text(
            scoreX + 48 + h * 16, scoreY + 32, h > nHole ? "-" : buf, 0xffffff,
            0, 0
          );
        }
      }

      // Prompt
      oldfont = vga_font(VGA_FONT_ALT_BOLD);
      vga_text(
        (VGA_WIDTH - 200) >> 1, y1 - 32, "Press RETURN to continue", 0xffffff,
        0, 0
      );
      vga_font(oldfont);

      // Process input
      ev = kbd_getKeyEvent();
      switch (ev.key) {
        case KEY_RETURN:
          // Show scoreboard for last hole
          if (nHole == settings->nHoles - 1) {
            gameState = GG_GAME_SCOREBOARD;
          } else {
            loop = 0;
          }
          break;
      }
    } else if (gameState == GG_GAME_SCOREBOARD) {
      uint16_t x0 = (VGA_WIDTH >> 1) - 192, x1 = x0 + 383;
      uint16_t y0 = (VGA_HEIGHT >> 1) - 96, y1 = y0 + 191;

      // Background
      vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_DARK, VGA_ALPHA_BLEND);
      vga_shade(x0, y0, x1, y1, UI_GREEN_DARK, 0);
      vga_frame(x0, y0, x1, y1, 0xffffff, 0);

      // Score counter (common to 1P-2P)
      uint16_t scoreX = (VGA_WIDTH - 232) >> 1;
      uint16_t scoreY = y0 + 96;
      vga_text(scoreX, scoreY, " Hole", 0xffffff, 0x000000, 0);
      vga_text(scoreX + 192, scoreY, "Total", 0xffffff, 0x000000, 0);

      for (uint32_t h = 0; h < settings->nHoles; h++) {
        sprintf(buf, "%u", h + 1);
        vga_text(scoreX + 48 + h * 16, scoreY, buf, 0xffffff, 0, 0);
        sprintf(buf, "%u", settings->scores[0][h]);
        vga_text(
          scoreX + 48 + h * 16, scoreY + 16, h > nHole ? "-" : buf, 0xffffff, 0,
          0
        );
      }

      sprintf(buf, "%u", settings->scores[0][MAX_HOLES]);
      vga_text(scoreX + 192, scoreY + 16, buf, 0xffffff, 0, 0);

      if (settings->nPlayers == 1) {
        // Title
        sprintf(buf, "Your score: %u", settings->scores[0][MAX_HOLES]);
        uint32_t textWidth = strlen(buf) * 12;

        oldfont = vga_font(VGA_FONT_LARGE);
        vga_text((VGA_WIDTH - textWidth) >> 1, y0 + 16, buf, 0xffffff, 0, 0);
        vga_font(oldfont);

        // Single player score
        vga_text(scoreX, scoreY + 16, "Score", 0xffffff, 0x000000, 0);
      } else if (settings->nPlayers == 2) {
        uint32_t winner =
          settings->scores[0][MAX_HOLES] > settings->scores[1][MAX_HOLES] ? 1
                                                                          : 2;

        // Title
        if (settings->scores[0][MAX_HOLES] != settings->scores[1][MAX_HOLES]) {
          sprintf(buf, "Player %u wins the game!", winner);
        } else {
          sprintf(buf, "Game draw!");
        }
        uint32_t textWidth = strlen(buf) * 12;

        oldfont = vga_font(VGA_FONT_LARGE);
        vga_text((VGA_WIDTH - textWidth) >> 1, y0 + 16, buf, 0xffffff, 0, 0);
        vga_font(oldfont);

        // Multiplayer scores
        vga_text(scoreX, scoreY + 16, "   P1", 0xffffff, 0x000000, 0);
        vga_text(scoreX, scoreY + 32, "   P2", 0xffffff, 0x000000, 0);

        for (uint32_t h = 0; h < settings->nHoles; h++) {
          sprintf(buf, "%u", settings->scores[1][h]);
          vga_text(
            scoreX + 48 + h * 16, scoreY + 32, h > nHole ? "-" : buf, 0xffffff,
            0, 0
          );
        }

        sprintf(buf, "%u", settings->scores[1][MAX_HOLES]);
        vga_text(scoreX + 192, scoreY + 32, buf, 0xffffff, 0, 0);
      }

      // Prompt
      oldfont = vga_font(VGA_FONT_ALT_BOLD);
      vga_text(
        (VGA_WIDTH - 160) >> 1, y1 - 32, "Press RETURN to end", 0xffffff, 0, 0
      );
      vga_font(oldfont);

      // Process input
      ev = kbd_getKeyEvent();
      switch (ev.key) {
        case KEY_RETURN:
          loop = 0;
          break;
      }
    }

    if (gameState != GG_GAME_RUNNING) {
      // Increment endgame animation timer
      t += ANIM_SPEED * frametime;

      // Move camera after endgame
      float tView = min(t * 0.004f, 1.0f);
      float angle = t * 0.003;
      while (angle > M_PI) angle -= 2.0f * M_PI;
      viewPos.y = lerp(10.0f, 6.0f, tView);
      viewPos.z = lerp(3.5f, 8.0f, tView);

      float4x4 view = mat_lookat(viewPos, viewTarget, viewUp);
      view = mmul(view, mat_rotationY(angle));

      gfx_setMatrix(GFX_MAT_VIEW, &view);
    }

    // Draw the frametime counter
    uint64_t fpsTimes100 = frametime == 0 ? 0 : 100000 / frametime;
    uint64_t fps = fpsTimes100 / 100;
    fpsTimes100 %= 100;

    sprintf(
      buf, "Frametime: %llums (%llu.%02llu fps)", frametime, fps, fpsTimes100
    );
    vga_text(0, 0, buf, 0xffffff, 0, VGA_TEXT_BG);

    vga_present();
  }

  return 0;
}

/*
 * Entry point for game
 */
int gg_startGame() {
  // Disable status bar drawing while application is active
  uint8_t statusEnabled = _syscall(SYS_STATUS_GET_ENABLED);
  _syscall(SYS_STATUS_SET_ENABLED, 0);

  // Initialize RNG
  pcg32_random_t rng;
  pcg32_srand(&rng, _syscall(SYS_TICKS), 1);

  // Make vertices for the hole mesh
  makeHoleMesh();

  // Load the capybara models
  gfx_parseObj(obj_capybase, v_base, n_base, vi_base, ni_base, &pc_base);
  gfx_parseObj(obj_capyface, v_face, n_face, vi_face, ni_face, &pc_face);
  gfx_parseObj(obj_capyclub, v_club, n_club, vi_club, ni_club, &pc_club);
  gfx_parseObj(obj_flag, v_flag, n_flag, vi_flag, ni_flag, &pc_flag);
  gfx_parseObj(obj_flagpole, v_pole, n_pole, vi_pole, ni_pole, &pc_pole);
  gfx_parseObj(obj_ball, v_ball, n_ball, vi_ball, ni_ball, &pc_ball);

  // Init deltatime timer
  totalTicks = _syscall(SYS_TICKS);

  // Set "alt" as the default font for the application
  vga_font_t oldfont = vga_font(VGA_FONT_ALT);

  // Main application loop
  while (1) {
    // Display the title screen
    gameSettings_t settings;
    settings.nPlayers = 1;
    settings.nHoles = DEFAULT_HOLES;
    for (int i = 0; i < settings.nPlayers; i++) {
      settings.scores[i][MAX_HOLES] = 0;
    }

    int play = showTitleScreen(&settings);
    if (!play) break;

    for (uint32_t hole = 0; hole < settings.nHoles; hole++) {
      int quit = playGame(&settings, hole, &rng);
      if (quit) break;
    }
  }

  // Restore font
  vga_font(oldfont);

  // Restore status bar enabled state
  _syscall(SYS_STATUS_SET_ENABLED, statusEnabled);

  return 0;
}
