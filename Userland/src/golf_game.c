#include <fpmath.h>
#include <golf_game.h>
#include <graphics.h>
#include <kbd.h>
#include <mem.h>
#include <print.h>
#include <process.h>
#include <rng.h>
#include <sound.h>
#include <status.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include <time.h>
#include <vga.h>

#define deg2rad(x) ((x) / 180.0f * M_PI)

#define VGA_WIDTH size.x
#define VGA_HEIGHT size.y
#define CENTER_X (VGA_WIDTH >> 1)
#define CENTER_Y (VGA_HEIGHT >> 1)

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
#define GRAVITY 0.03f
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

  float inv_mass;// para colisions y podria afectar el gama
  // si usamos la inversa de la masa nos ahoramos la division de float

  float drag;// friction/arraste // NO es exacto, es una idea mas simple

  float angle;
} physics_object_t;

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
  uint32_t n_players;
  uint32_t n_holes;

  uint32_t scores[MAX_PLAYERS][MAX_HOLES + 1];
} game_settings_t;

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
} gg_menu_option_t;

typedef enum {
  GG_PAUSE_RESUME,
  GG_PAUSE_SKIP,
  GG_PAUSE_QUIT,
} gg_pause_option_t;

typedef enum {
  GG_GAME_RUNNING,
  GG_GAME_END,
  GG_GAME_SCOREBOARD,
} gg_game_state_t;

typedef enum {
  GG_ANIM_NONE,
  GG_ANIM_WIN,
  GG_ANIM_LOSE,
} gg_player_anim_t;

/*
 * Bitmap image data
 */
static const uint8_t *titlescreen_logo = (uint8_t *) 0x800000;

/*
 * OBJ strings for 3D models
 */
static const void *obj_capybase = (void *) 0x803000;
static const void *obj_capyface = (void *) 0x806000;
static const void *obj_capyclub = (void *) 0x806800;
static const void *obj_flag = (void *) 0x806C00;
static const void *obj_flagpole = (void *) 0x806E00;
static const void *obj_ball = (void *) 0x807000;

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
static const char *score_names[] = {"Hole in ZERO!", "Hole in One!", "Eagle",
                                    "Birdie",        "Par",          "Bogey",
                                    "Double Bogey",  "over Par"};
static const char *score_texts[] = {
  "How??",        "YOU'RE WINNER",      "Great!",
  "Nicely done!", "Not bad, not great", "Almost there...",
  "git gud",      "Major Skill Issue",
};

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
static uint32_t pc_walls = 8;


typedef struct {
  /*
   * 3D model data
   */
  float3 *v_base;
  float3 *n_base;
  uint32_t *vi_base;
  uint32_t *ni_base;

  float3 *v_face;
  float3 *n_face;
  uint32_t *vi_face;
  uint32_t *ni_face;

  float3 *v_club;
  float3 *n_club;
  uint32_t *vi_club;
  uint32_t *ni_club;

  float3 *v_flag;
  float3 *n_flag;
  uint32_t *vi_flag;
  uint32_t *ni_flag;

  float3 *v_pole;
  float3 *n_pole;
  uint32_t *vi_pole;
  uint32_t *ni_pole;

  float3 *v_ball;
  float3 *n_ball;
  uint32_t *vi_ball;
  uint32_t *ni_ball;

  uint32_t pc_base, pc_face, pc_club, pc_flag, pc_pole, pc_ball;

  /*
   * Frametime counter
   */
  uint64_t frametime;
  uint64_t total_ticks;
  int show_fps;

  /*
   * Background framebuffers for optimized pre-rendering
   */
  gfx_framebuffer_t bg_framebuffer;
  gfx_depthbuffer_t bg_depthbuffer;

  gfx_context_t ctx;
} game_t;

/*
 * Helper functions
 */
static inline void update_timer(game_t *game) {
  game->frametime = time() - game->total_ticks;
  game->total_ticks += game->frametime;
}

static inline void make_hole_mesh() {
  for (int i = 0; i < 7; i++) {
    float a = -M_PI + 2.0f * i / 7.0f * M_PI;
    float3 v = {cos(a), 0.0f, sin(a)};
    v_hole[i] = v;
  }
}

static inline float random_float(pcg32_random_t *rng) {
  // Convert a random integer to a float in the 0-1 range
  return (pcg32_rand(rng) >> 8) * 0x1p-24f;// funky hex float notation
}

static void
accelerate_object(game_t *game, physics_object_t *obj, vector_t *dir) {
  // Add velocity
  obj->vx += dir->x;
  obj->vy += dir->y;

  // Cap velocity
  float v = sqrt(sqr(obj->vx) + sqr(obj->vy));
  float vmax = VMAX / game->frametime;
  if (v > vmax) {
    float factor = vmax / v;
    obj->vx *= factor;
    obj->vy *= factor;
  }
}

static void
update_player(game_t *game, physics_object_t *obj, keycode_t keys[4]) {
  int up = kbd_keydown(keys[0]);
  int down = kbd_keydown(keys[1]);
  int right = kbd_keydown(keys[2]);
  int left = kbd_keydown(keys[3]);

  if (right) {
    obj->angle += TURNS_SPEED * game->frametime;
    if (obj->angle > M_PI) obj->angle -= 2.0f * M_PI;
  }
  if (left) {
    obj->angle -= TURNS_SPEED * game->frametime;
    if (obj->angle < -M_PI) obj->angle += 2.0f * M_PI;
  }
  if (up) {
    vector_t dir;
    dir.x = ACCELERATION * game->frametime * cos(obj->angle);
    dir.y = ACCELERATION * game->frametime * sin(obj->angle);
    accelerate_object(game, obj, &dir);
  }
  if (down) {
    obj->vx *= BRAKING;
    obj->vy *= BRAKING;
  }
}

static void update_object(game_t *game, physics_object_t *obj) {
  float oldx = obj->x;
  float oldy = obj->y;

  float dragMlt = 1.0f - obj->drag * 0.001f;
  dragMlt = max(0.0f, dragMlt);

  // ok this is stupid BUT
  for (uint32_t i = 0; i < game->frametime; i++) {
    obj->vx *= dragMlt;
    obj->vy *= dragMlt;
  }

  obj->x += obj->vx * VMUL * game->frametime;
  obj->y += obj->vy * VMUL * game->frametime;

  if ((obj->x - obj->size) < 0.0f || (obj->x + obj->size) > FIELD_WIDTH) {
    obj->x = oldx;
    obj->vx = -(obj->vx);
  }
  if ((obj->y - obj->size) < 0.0f || (obj->y + obj->size) > FIELD_HEIGHT) {
    obj->y = oldy;
    obj->vy = -(obj->vy);
  }
}

static inline int
check_collision(physics_object_t *a, physics_object_t *b, vector_t *dir) {
  float difx = b->x - a->x;
  float dify = b->y - a->y;
  float dist = sqrt(sqr(difx) + sqr(dify));

  if (dist <= b->size + a->size) {
    dir->x = (difx);
    dir->y = (dify);
    return 1;
  } else {
    return 0;
  }
}

static int
do_collision(game_t *game, physics_object_t *a, physics_object_t *b) {
  float va = abs(a->vx) + abs(a->vy);
  float vb = abs(b->vx) + abs(b->vy);

  vector_t dir = {0};
  if (check_collision(a, b, &dir)) {
    vector_t dirb = dir;
    dirb.x *= (b->inv_mass * (va + vb));
    dirb.y *= (b->inv_mass * (va + vb));
    accelerate_object(game, b, &dirb);

    dir.x *= -((a->inv_mass * (va + vb)));
    dir.y *= -((a->inv_mass * (va + vb)));
    accelerate_object(game, a, &dir);

    return 1;
  }

  return 0;
}

static void
apply_gravity(game_t *game, terrain_t *terrain, physics_object_t *obj) {
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
  accelerate_object(game, obj, &a);
}

static int check_hole(physics_object_t *obj, hole_t *hole) {
  float difx = hole->x - obj->x;
  float dify = hole->y - obj->y;
  float distsqr = sqr(difx) + sqr(dify);

  if (distsqr <= sqr(hole->size + obj->size)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Terrain generation
 */
static float do_hill(int curx, int cury, int x, int y, float size) {
  float difx = x - curx;
  float dify = y - cury;
  float dist = sqrt(sqr(difx) + sqr(dify));

  if (dist <= size) {
    return (1 - dist / size) * TERRAIN_HILL_HEIGHT;
  } else {
    return 0;
  }
}

static void generate_terrain(terrain_t *terrain, pcg32_random_t *rng) {
  // Generate terrain vertices
  float fy[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    fy[i] = ((float) (pcg32_rand(rng) % 10)) / 10;
  }
  float fx[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    fx[i] = ((float) (pcg32_rand(rng) % 10)) / 10;
  }

  float offsety[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    offsety[i] = ((float) (pcg32_rand(rng) % 5) / 5) * 2 * M_PI;
  }
  float offsetx[TERRAIN_N_WAVES];
  for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
    offsetx[i] = ((float) (pcg32_rand(rng) % 5) / 5) * 2 * M_PI;
  }

  int hillx = (pcg32_rand(rng) % TERRAIN_SIZE_X);
  int hilly = (pcg32_rand(rng) % TERRAIN_SIZE_Y);

  int pitx = (pcg32_rand(rng) % TERRAIN_SIZE_X);
  int pity = (pcg32_rand(rng) % TERRAIN_SIZE_Y);

  for (int y = 0; y <= TERRAIN_SIZE_Y; y++) {
    for (int x = 0; x <= TERRAIN_SIZE_X; x++) {
      float height = 0;
      for (int i = 0; i < TERRAIN_N_WAVES; ++i) {
        height +=
          ((sin(fit_to_pi((x * TERRAIN_SIZE_UNITS_X) * fy[i] + offsety[i])) *
            TERRAIN_NOISE_MAX) +
           (sin(fit_to_pi((y * TERRAIN_SIZE_UNITS_X) * fx[i] + offsetx[i])) *
            TERRAIN_NOISE_MAX)) /
          TERRAIN_N_WAVES;
      }

      height += do_hill(x, y, hillx, hilly, TERRAIN_HILL_WIDTH);

      height -= do_hill(x, y, pitx, pity, TERRAIN_HILL_WIDTH);

      if ((x == FIELD_WIDTH || x == 0) || (y == FIELD_HEIGHT || y == 0)) {
        height = max(height, 0.0f) + TERRAIN_NOISE_MAX / 2;
      }

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

/*
 * Rendering
 */
static void setup_game_render(game_t *game, float4x4 *view) {
  // Set half render resolution for the game for increased framerate
  gfx_set_flag(game->ctx, GFX_HALFRES, 1);

  // Set up view and projection matrices
  gfx_set_matrix(game->ctx, GFX_MAT_VIEW, view);

  uint2 size = gfx_get_resolution(game->ctx);
  float aspect = (float) size.x / size.y;
  float fov_degrees = 75.0f;
  float4x4 projection =
    mat_perspective(deg2rad(fov_degrees), aspect, 0.1f, 100.0f);
  gfx_set_matrix(game->ctx, GFX_MAT_PROJECTION, &projection);

  // Set up lighting
  float3 light = {-2, 3, -2};
  float3 lightcolor = {1.80f, 1.75f, 1.60f};
  float3 ambient = vmuls(lightcolor, 0.5f);

  gfx_light_t light_type = GFX_LIGHT_DIRECTIONAL;
  gfx_set_light_type(game->ctx, light_type);
  gfx_set_light(game->ctx, GFX_LIGHT_POSITION, &light);
  gfx_set_light(game->ctx, GFX_AMBIENT_LIGHT, &ambient);
  gfx_set_light(game->ctx, GFX_LIGHT_COLOR, &lightcolor);
}

static void render_terrain(game_t *game, terrain_t *terrain) {
  // Draw background
  uint2 size = gfx_get_resolution(game->ctx);
  proc_set_external_framebuffer(gfx_get_framebuffer(game->ctx));
  vga_gradient(
    0, 0, VGA_WIDTH - 1, VGA_HEIGHT - 1, colors(0x1a32e6, 0x07d0f8), VGA_GRAD_V
  );
  vga_shade(0, 0, VGA_WIDTH - 1, VGA_HEIGHT - 1, 0x50000080, VGA_ALPHA_BLEND);
  proc_set_external_framebuffer(NULL);

  // Set transform to identity
  float4x4 model = mat_scale(1, 1, 1);
  gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

  float3 terrain_color = {0.18, 0.37, 0.05};

  // Draw the terrain
  gfx_draw_primitives_indexed(
    game->ctx, (float3 *) terrain->v, (float3 *) terrain->normals,
    terrain->indices, terrain->indices, TERRAIN_SIZE_X * TERRAIN_SIZE_Y * 2,
    terrain_color
  );

  // Draw terrain bound walls
  float hmax = (TERRAIN_HILL_HEIGHT + TERRAIN_NOISE_MAX + 0.5f);
  model = mat_scale(FIELD_WIDTH * 0.5f, hmax * 0.5f, FIELD_HEIGHT * 0.5f);
  model = mmul(mat_translation(0.0f, hmax * 0.25f, 0.0f), model);
  gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

  float3 wall_color = {0.37, 0.18, 0.05};
  gfx_draw_primitives_indexed(
    game->ctx, v_walls, n_walls, vi_walls, ni_walls, pc_walls, wall_color
  );
}

static float get_terrain_height_at(terrain_t *terrain, float fx, float fy) {
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

static void render_player(
  game_t *game, physics_object_t *player, uint32_t i, gg_player_anim_t anim,
  float t, terrain_t *terrain
) {
  float angle = -player->angle - M_PI * 0.5f;
  if (angle < -M_PI) angle += M_PI * 2.0f;

  float height = get_terrain_height_at(terrain, player->x, player->y);

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
  gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

  // Draw the capybara
  gfx_draw_primitives_indexed(
    game->ctx, game->v_face, game->n_face, game->vi_face, game->ni_face,
    game->pc_face, color_face
  );
  gfx_draw_primitives_indexed(
    game->ctx, game->v_base, game->n_base, game->vi_base, game->ni_base,
    game->pc_base, color_base[i]
  );
  gfx_draw_primitives_indexed(
    game->ctx, game->v_club, game->n_club, game->vi_club, game->ni_club,
    game->pc_club, color_club
  );
}

static void render_ball(
  game_t *game, physics_object_t *ball, uint32_t i, terrain_t *terrain
) {
  float height = get_terrain_height_at(terrain, ball->x, ball->y);

  float4x4 model = mat_translation(
    ball->x - FIELD_WIDTH * 0.5f, height, ball->y - FIELD_HEIGHT * 0.5f
  );
  gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

  gfx_draw_primitives_indexed(
    game->ctx, game->v_ball, game->n_ball, game->vi_ball, game->ni_ball,
    game->pc_ball, color_ball[i]
  );
}

static void render_hole(game_t *game, hole_t *hole, terrain_t *terrain) {
  float height = get_terrain_height_at(terrain, hole->x, hole->y);
  float4x4 translation = mat_translation(
    hole->x - FIELD_WIDTH * 0.5f, height, hole->y - FIELD_HEIGHT * 0.5f
  );

  float4x4 model = mmul(translation, mat_scale(hole->size, 1.0f, hole->size));
  gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

  float3 color_hole = {0.0f, 0.0f, 0.0f};
  gfx_set_flag(game->ctx, GFX_DEPTH_TEST | GFX_DEPTH_WRITE, 0);
  gfx_draw_primitives_indexed(
    game->ctx, v_hole, NULL, vi_hole, vi_hole, 5, color_hole
  );
  gfx_set_flag(game->ctx, GFX_DEPTH_TEST | GFX_DEPTH_WRITE, 1);

  model = mmul(translation, mat_rotationY(M_PI * 0.75f));
  gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

  float3 color_pole = {0.80f, 0.80f, 0.80f};
  float3 color_flag = {0.92f, 0.20f, 0.24f};
  gfx_draw_primitives_indexed(
    game->ctx, game->v_pole, game->n_pole, game->vi_pole, game->vi_pole,
    game->pc_pole, color_pole
  );
  gfx_draw_primitives_indexed(
    game->ctx, game->v_flag, game->n_flag, game->vi_flag, game->vi_flag,
    game->pc_flag, color_flag
  );
}

/*
 * Show the title screen/main menu and handle input.
 * Returns when the game should be started.
 */
static int show_title_screen(game_t *game, game_settings_t *settings) {
  gg_screen_t screen = GG_SCREEN_TITLE;
  float a = 0.0f, capy_angle = (M_PI * -0.75);
  uint32_t text_blink_timer = 0;

  /*
   * Menu state
   */
  int menu_item = 0;
  static const char *menu_strings[] = {
    "Play (1 player)",
    "Play (2 player)",
    " Quit to Shell ",
  };
  static const char *setup_strings[] = {
    " Holes: X ",
    "Start Game",
    "   Back   ",
  };
  uint32_t menu_item_count = sizeof(menu_strings) / sizeof(menu_strings[0]);
  uint32_t setup_item_count = sizeof(setup_strings) / sizeof(setup_strings[0]);

  /*
   * Graphics setup
   */

  // Set full render resolution for the main menu, we're not rendering much
  gfx_set_flag(game->ctx, GFX_HALFRES, 1);

  // Set up view and projection matrices
  float3 pos = {0, 1, 3.5f};
  float3 target = {0, 0, 0};
  float3 up = {0, 1, 0};

  float4x4 view = mat_lookat(pos, target, up);
  gfx_set_matrix(game->ctx, GFX_MAT_VIEW, &view);

  uint2 size = gfx_get_resolution(game->ctx);
  size.x *= 2;
  size.y *= 2;
  float aspect = (float) size.x / size.y;
  float fov_degrees = 75.0f;
  float4x4 projection =
    mat_perspective(deg2rad(fov_degrees), aspect, 0.1f, 10.0f);
  gfx_set_matrix(game->ctx, GFX_MAT_PROJECTION, &projection);

  // Set up lighting
  float3 light = {-1, 3, -1};
  float3 lightcolor = {2, 2, 2};
  float3 ambient = vmuls(lightcolor, 0.5f);

  gfx_light_t lightType = GFX_LIGHT_DIRECTIONAL;
  gfx_set_light_type(game->ctx, lightType);
  gfx_set_light(game->ctx, GFX_LIGHT_POSITION, &light);
  gfx_set_light(game->ctx, GFX_AMBIENT_LIGHT, &ambient);
  gfx_set_light(game->ctx, GFX_LIGHT_COLOR, &lightcolor);

  /*
   * Draw loop
   */
  while (1) {
    // Update the timer
    update_timer(game);

    // Clear graphics frame and depth buffers
    gfx_clear(game->ctx, 0);

    // Pass the graphics framebuffer to the VGA driver for 2D drawing
    // This lets us draw shapes behind the 3d graphics
    proc_set_external_framebuffer(gfx_get_framebuffer(game->ctx));

    // Draw a nice background
    // Sky
    vga_gradient(
      0, 0, (VGA_WIDTH >> 1) - 1, (VGA_HEIGHT >> 2) - 1,
      colors(0x1a32e6, 0x07d0f8), VGA_GRAD_V
    );
    vga_shade(
      0, 0, (VGA_WIDTH >> 1) - 1, (VGA_HEIGHT >> 2) - 1, 0x50000080,
      VGA_ALPHA_BLEND
    );

    // Grass
    vga_gradient(
      0, VGA_HEIGHT >> 2, (VGA_WIDTH >> 1) - 1, (VGA_HEIGHT >> 1) - 1,
      colors(0x83fa00, 0x008f00), VGA_GRAD_V
    );

    // 3D graphics
    // Create a model matrix and set it
    uint32_t n_capybaras = menu_item == GG_MM_MULTIPLAYER ? 2 : 1;
    if (screen == GG_SCREEN_SETUP) n_capybaras = settings->n_players;
    for (int i = 0; i < n_capybaras; i++) {
      float x = n_capybaras == 1 ? 0.0f : -1.5f + i * 3.0f;
      float4x4 model = mat_rotationY(capy_angle);
      model = mmul(mat_translation(x, -1.3f, 0), model);
      gfx_set_matrix(game->ctx, GFX_MAT_MODEL, &model);

      // Draw the capybara
      gfx_draw_primitives_indexed(
        game->ctx, game->v_face, game->n_face, game->vi_face, game->ni_face,
        game->pc_face, color_face
      );
      gfx_draw_primitives_indexed(
        game->ctx, game->v_base, game->n_base, game->vi_base, game->ni_base,
        game->pc_base, color_base[i]
      );
      gfx_draw_primitives_indexed(
        game->ctx, game->v_club, game->n_club, game->vi_club, game->ni_club,
        game->pc_club, color_club
      );
    }

    // Present gfx buffer to VGA main framebuffer
    gfx_present(game->ctx);

    // Restore the default VGA framebuffer to draw on top of the 3D graphics
    proc_set_external_framebuffer(NULL);

    // Draw the title logo (it floats!)
    vga_bitmap(
      CENTER_X - 256, CENTER_Y - 256 - 18 * sin(a), titlescreen_logo, 2,
      VGA_ALPHA_BLEND
    );

    // Draw UI text
    if (screen == GG_SCREEN_TITLE &&
        text_blink_timer > (TITLE_TEXT_BLINK_MS >> 1)) {
      vga_font_t oldfont = vga_font(VGA_FONT_ALT_BOLD);
      vga_text(
        CENTER_X - 88, CENTER_Y + 116, "Press any key to start", 0xffffff, 0,
        VGA_TEXT_BG
      );
      vga_font(oldfont);
    }

    if (screen == GG_SCREEN_PLAYERSELECT) {
      uint16_t x0 = CENTER_X - 128, y0 = CENTER_Y + 16;
      uint16_t x1 = x0 + 255, y1 = y0 + 199;

      vga_shade(x0, y0, x1, y1, 0, 0);
      vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_LIGHT, VGA_ALPHA_BLEND);
      vga_frame(x0, y0, x1, y1, 0xffffff, 0);

      vga_font_t oldfont = vga_font(VGA_FONT_LARGE);
      vga_text(CENTER_X - 54, y0 + 16, "MAIN MENU", 0xffffff, 0, 0);
      vga_font(oldfont);

      for (int i = 0; i < menu_item_count; i++) {
        if (i == menu_item) {
          vga_rect(
            CENTER_X - 84, y0 + 56 + 24 * i - 4, CENTER_X + 83,
            y0 + 56 + 24 * i + 19, UI_GREEN_HIGHLIGHT, 0
          );
          vga_frame(
            CENTER_X - 84, y0 + 56 + 24 * i - 4, CENTER_X + 83,
            y0 + 56 + 24 * i + 19, 0xffffff, 0
          );
        }
        vga_text(
          CENTER_X - 60, y0 + 56 + 24 * i, menu_strings[i], 0xffffff, 0, 0
        );
      }
    }

    if (screen == GG_SCREEN_SETUP) {
      uint16_t x0 = CENTER_X - 128, y0 = CENTER_Y + 16;
      uint16_t x1 = x0 + 255, y1 = y0 + 199;

      vga_shade(x0, y0, x1, y1, 0, 0);
      vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_LIGHT, VGA_ALPHA_BLEND);
      vga_frame(x0, y0, x1, y1, 0xffffff, 0);

      vga_font_t oldfont = vga_font(VGA_FONT_LARGE);
      vga_text(CENTER_X - 60, y0 + 16, "GAME SETUP", 0xffffff, 0, 0);
      vga_font(oldfont);

      for (int i = 0; i < setup_item_count; i++) {
        if (i == menu_item) {
          vga_rect(
            CENTER_X - 84, y0 + 56 + 24 * i - 4, CENTER_X + 83,
            y0 + 56 + 24 * i + 19, UI_GREEN_HIGHLIGHT, 0
          );
          vga_frame(
            CENTER_X - 84, y0 + 56 + 24 * i - 4, CENTER_X + 83,
            y0 + 56 + 24 * i + 19, 0xffffff, 0
          );
        }
        if (i == 0) {
          char buf[20];
          sprintf(buf, " Holes: %u ", settings->n_holes);
          vga_text(CENTER_X - 40, y0 + 56, buf, 0xffffff, 0, 0);
        } else {
          vga_text(
            CENTER_X - 40, y0 + 56 + 24 * i, setup_strings[i], 0xffffff, 0, 0
          );
        }
      }
    }

    vga_text(
      VGA_WIDTH - 200, VGA_HEIGHT - 32, "(c) 1998 TONKATSU GAMES", 0xffffff, 0,
      0
    );

    // Draw everything to screen
    vga_present();

    // Update vars
    a += 0.005f * game->frametime;
    if (a > M_PI) a -= 2.0f * M_PI;
    capy_angle -= 0.5f * ANIM_SPEED * game->frametime;
    if (capy_angle < -M_PI) capy_angle += 2.0f * M_PI;
    text_blink_timer =
      (text_blink_timer + game->frametime) % TITLE_TEXT_BLINK_MS;

    // Process input
    if (kbd_poll_events()) {
      if (screen == GG_SCREEN_TITLE) {
        screen = GG_SCREEN_PLAYERSELECT;
      } else if (screen == GG_SCREEN_PLAYERSELECT) {
        if (kbd_keypressed(KEY_ARROW_UP)) {
          menu_item = menu_item == 0 ? menu_item_count - 1 : menu_item - 1;
        } else if (kbd_keypressed(KEY_ARROW_DOWN)) {
          menu_item = (menu_item + 1) % menu_item_count;
        } else if (kbd_keypressed(KEY_RETURN)) {
          if (menu_item == GG_MM_QUIT) return 0;

          settings->n_players = menu_item + 1;
          menu_item = 0;
          screen = GG_SCREEN_SETUP;
        }
      } else if (screen == GG_SCREEN_SETUP) {
        if (kbd_keypressed(KEY_ARROW_UP)) {
          menu_item = menu_item == 0 ? setup_item_count - 1 : menu_item - 1;
        } else if (kbd_keypressed(KEY_ARROW_DOWN)) {
          menu_item = (menu_item + 1) % setup_item_count;
        } else if (kbd_keypressed(KEY_ARROW_LEFT)) {
          settings->n_holes = max(1, settings->n_holes - 1);
        } else if (kbd_keypressed(KEY_ARROW_RIGHT)) {
          settings->n_holes = min(MAX_HOLES, settings->n_holes + 1);
        } else if (kbd_keypressed(KEY_RETURN)) {
          if (menu_item == GG_SM_BACK) {
            menu_item = 0;
            screen = GG_SCREEN_PLAYERSELECT;
          } else if (menu_item == GG_SM_START) {
            return 1;
          }
        }
      }
    }
  }
}

static gg_pause_option_t show_pause_menu(game_t *game) {
  int exit = 0;
  gg_pause_option_t selected = 0;

  static const char *option_strings[] = {
    "   Resume game   ",
    "    Skip hole    ",
    "Quit to main menu",
  };
  uint32_t option_count = sizeof(option_strings) / sizeof(option_strings[0]);

  uint2 size = gfx_get_resolution(game->ctx);
  size.x *= 2;
  size.y *= 2;

  while (!exit) {
    update_timer(game);

    uint16_t x0 = CENTER_X - 128, x1 = x0 + 255;
    uint16_t y0 = CENTER_Y - 96, y1 = y0 + 191;

    // Use current game graphics as background
    gfx_present(game->ctx);

    // Background
    vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_DARK, VGA_ALPHA_BLEND);
    vga_shade(x0, y0, x1, y1, UI_GREEN_DARK, 0);
    vga_frame(x0, y0, x1, y1, 0xffffff, 0);

    // Title
    vga_font_t oldfont = vga_font(VGA_FONT_LARGE);
    vga_text((VGA_WIDTH - 72) >> 1, y0 + 16, "PAUSED", 0xffffff, 0, 0);
    vga_font(oldfont);

    // Options
    for (int i = 0; i < option_count; i++) {
      if (i == selected) {
        vga_rect(
          CENTER_X - 84, y0 + 56 + 24 * i - 4, CENTER_X + 83,
          y0 + 56 + 24 * i + 19, UI_GREEN_HIGHLIGHT, 0
        );
        vga_frame(
          CENTER_X - 84, y0 + 56 + 24 * i - 4, CENTER_X + 83,
          y0 + 56 + 24 * i + 19, 0xffffff, 0
        );
      }
      vga_text(
        (VGA_WIDTH - 136) >> 1, y0 + 56 + 24 * i, option_strings[i], 0xffffff,
        0, 0
      );
    }

    vga_present();

    // Keyboard input
    if (kbd_poll_events()) {
      if (kbd_keypressed(KEY_ARROW_UP)) {
        selected = selected == 0 ? option_count - 1 : selected - 1;
      } else if (kbd_keypressed(KEY_ARROW_DOWN)) {
        selected = (selected + 1) % option_count;
      } else if (kbd_keypressed(KEY_RETURN)) {
        exit = 1;
      }
    }
  }

  return selected;
}

static int play_game(
  game_t *game, game_settings_t *settings, uint32_t nHole, pcg32_random_t *rng
) {
  // Keymaps per player
  static keycode_t keys[2][4] = {
    {KEY_W, KEY_S, KEY_D, KEY_A},
    {KEY_ARROW_UP, KEY_ARROW_DOWN, KEY_ARROW_RIGHT, KEY_ARROW_LEFT}
  };

  gg_game_state_t game_state = GG_GAME_RUNNING;

  /*
   * Set up game objects
   */
  terrain_t terrain;
  generate_terrain(&terrain, rng);

  physics_object_t players[MAX_PLAYERS];
  physics_object_t balls[MAX_PLAYERS];
  int ball_in_play[MAX_PLAYERS];
  uint32_t hits[MAX_PLAYERS];
  uint32_t iframes[MAX_PLAYERS];// Hit counter debounce timer
  gg_player_anim_t anim[MAX_PLAYERS];

  for (int i = 0; i < settings->n_players; i++) {
    players[i].x = FIELD_WIDTH * (random_float(rng) * 0.8f + 0.1f);
    players[i].y = FIELD_HEIGHT * (random_float(rng) * 0.8f + 0.1f);
    players[i].vx = 0.0f;
    players[i].vy = 0.0f;
    players[i].drag = 22.0f;
    players[i].size = 0.7f;
    players[i].inv_mass = 0.1f;
    players[i].angle = 0.0f;

    balls[i].x = FIELD_WIDTH * (random_float(rng) * 0.8f + 0.1f);
    balls[i].y = FIELD_HEIGHT * (random_float(rng) * 0.8f + 0.1f);
    balls[i].vx = 0.0f;
    balls[i].vy = 0.0f;
    balls[i].drag = 12.0f;
    balls[i].size = 0.1f;
    balls[i].inv_mass = 1.0f;

    hits[i] = 0;
    iframes[i] = 0;
    ball_in_play[i] = 1;
    anim[i] = GG_ANIM_NONE;
  }

  hole_t hole = {0};
  hole.x = FIELD_WIDTH * (random_float(rng) * 0.8f + 0.1f);
  hole.y = FIELD_HEIGHT * (random_float(rng) * 0.8f + 0.1f);
  hole.size = 0.5f;

  // Setup game graphics
  float3 view_pos = {0, 10.0f, 3.5f};
  float3 view_target = {0, 0, 0};
  float3 view_up = {0, 1, 0};

  float4x4 view = mat_lookat(view_pos, view_target, view_up);
  setup_game_render(game, &view);

  /*
   * Pre-render the terrain to a separate frame and depth buffer
   * Terrain rendering is by far the most expensive, and the camera doesn't
   * move during gameplay, so we can gain a huge perf boost by pre-rendering
   * terrain and reusing the frame and depth buffers.
   * This literally multiplies framerate five times!
   */
  gfx_set_buffers(game->ctx, game->bg_framebuffer, game->bg_depthbuffer);
  gfx_clear(game->ctx, 0);
  render_terrain(game, &terrain);
  gfx_set_buffers(game->ctx, NULL, NULL);

  /*
   * Game loop
   */
  int loop = 1;
  kbd_event_t ev = {0};
  float t = 0;// Timer for win/lose animation
  while (loop) {
    // Update the timer
    update_timer(game);

    // Process input and physics only when the game is running
    if (game_state == GG_GAME_RUNNING) {
      // Update keyboard input
      kbd_poll_events();
      if (kbd_keypressed(KEY_F12)) game->show_fps = !game->show_fps;

      /*
       * Physics and gameplay update
       */
      for (int i = 0; i < settings->n_players; i++) {
        // Apply inputs and update player physics
        update_player(game, &players[i], keys[i]);
        update_object(game, &players[i]);

        // Apply gravity and update ball physics
        if (ball_in_play[i]) {
          apply_gravity(game, &terrain, &balls[i]);
          update_object(game, &balls[i]);
        }

        // Collisions
        // Number of collisions scales with N^2, this is fine with just two players
        for (int j = 0; j < settings->n_players; j++) {
          // Player-ball collisions
          if (ball_in_play[j]) {
            int hit = do_collision(game, &players[i], &balls[j]);

            // Increase hit counter only when player hits their own ball
            if (hit && i == j && iframes[i] == 0) {
              hits[i]++;
              iframes[i] = HIT_DEBOUNCE_MS;
              sound_ball_hit();
            } else if (hit && i != j) {
              sound_ball_hit();// Se puede poner otro sonido
            }
          }

          // Player-player and ball-ball collisions
          if (i != j) {
            do_collision(game, &players[i], &players[j]);
            if (ball_in_play[i] && ball_in_play[j])
              do_collision(game, &balls[i], &balls[j]);
          }
        }

        // Check win condition
        if (ball_in_play[i] && check_hole(&balls[i], &hole)) {
          // Remove the ball from the playfield
          ball_in_play[i] = 0;

          // If no balls are left, end the game
          int end_game = 1;
          for (int j = 0; j < settings->n_players; j++) {
            if (ball_in_play[j]) end_game = 0;
          }

          if (end_game) {
            game_state = GG_GAME_END;

            // Calculate scores
            for (int j = 0; j < settings->n_players; j++) {
              settings->scores[j][nHole] =
                (hits[j] > PAR + 2) ? 0 : (PAR + 3 - hits[j]);
              settings->scores[j][MAX_HOLES] += settings->scores[j][nHole];
            }

            // Set game end player animations
            if (settings->n_players == 1) {
              anim[0] = hits[0] > PAR ? GG_ANIM_LOSE : GG_ANIM_WIN;
            } else if (settings->n_players == 2) {
              anim[0] = hits[0] > hits[1] ? GG_ANIM_LOSE : GG_ANIM_WIN;
              anim[1] = hits[1] > hits[0] ? GG_ANIM_LOSE : GG_ANIM_WIN;
            }

            break;
          }
        }

        // Update hit timers
        iframes[i] =
          iframes[i] <= game->frametime ? 0 : iframes[i] - game->frametime;
      }

      if (kbd_keypressed(KEY_ESCAPE)) {
        gg_pause_option_t option = show_pause_menu(game);
        if (option == GG_PAUSE_SKIP) {
          game_state = GG_GAME_END;

          // Set scores to zero for this hole
          for (int j = 0; j < settings->n_players; j++) {
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
    if (game_state != GG_GAME_RUNNING) {
      // On endgame the camera *does* move, so we need to actually render the terrain
      gfx_clear(game->ctx, 0);
      render_terrain(game, &terrain);
    } else {
      // While playing, we can reuse the background we pre-rendered at the start
      gfx_copy(game->ctx, NULL, game->bg_framebuffer);
      gfx_depthcopy(game->ctx, NULL, game->bg_depthbuffer);
    }

    render_hole(game, &hole, &terrain);

    for (int i = 0; i < settings->n_players; i++) {
      render_player(game, &players[i], i, anim[i], t, &terrain);
      if (ball_in_play[i])
        render_ball(
          game, &balls[i], settings->n_players == 1 ? 0 : i + 1, &terrain
        );
    }

    gfx_present(game->ctx);

    /*
     * Draw UI
     */
    char buf[64];
    uint2 size = gfx_get_resolution(game->ctx);
    size.x *= 2;
    size.y *= 2;

    // HUD background
    vga_gradient(
      0, 0, VGA_WIDTH - 1, 63, colors(UI_GREEN_LIGHT, UI_GREEN_DARK), 0
    );
    vga_shade(0, 0, VGA_WIDTH - 1, 63, UI_GREEN_DARK, 0);
    vga_line(0, 63, VGA_WIDTH - 1, 63, 0xffffff, 0);
    vga_line(VGA_WIDTH >> 1, 0, VGA_WIDTH >> 1, 63, 0xffffff, 0);

    vga_rect(CENTER_X - 32, 12, CENTER_X + 31, 51, UI_GREEN_DARK, 0);
    vga_frame(CENTER_X - 32, 12, CENTER_X + 31, 51, 0xffffff, 0);

    // Text
    vga_font_t oldfont = vga_font(VGA_FONT_ALT_BOLD);
    sprintf(buf, "Hole %u", nHole + 1);
    vga_text(CENTER_X - 24, 16, buf, 0xffffff, 0x000000, 0);
    vga_font(oldfont);
    sprintf(buf, "Par: %u", PAR);
    vga_text(CENTER_X - 24, 32, buf, 0xffffff, 0x000000, 0);

    vga_text(24, 16, "PLAYER 1", 0xffffff, 0x000000, 0);
    sprintf(buf, "Hits: %u", hits[0]);
    vga_text(24, 32, buf, 0xffffff, 0x000000, 0);

    vga_text(104, 16, " Hole", 0xffffff, 0x000000, 0);
    vga_text(104, 32, "Score", 0xffffff, 0x000000, 0);

    for (uint32_t h = 0; h < settings->n_holes; h++) {
      sprintf(buf, "%u", h + 1);
      vga_text(152 + h * 16, 16, buf, 0xffffff, 0, 0);
      sprintf(buf, "%u", settings->scores[0][h]);
      vga_text(152 + h * 16, 32, h >= nHole ? "-" : buf, 0xffffff, 0, 0);
    }

    if (settings->n_players > 1) {
      vga_text(CENTER_X + 56, 16, "PLAYER 2", 0xffffff, 0x000000, 0);
      sprintf(buf, "Hits: %u", hits[1]);
      vga_text(CENTER_X + 56, 32, buf, 0xffffff, 0x000000, 0);

      vga_text(CENTER_X + 136, 16, " Hole", 0xffffff, 0x000000, 0);
      vga_text(CENTER_X + 136, 32, "Score", 0xffffff, 0x000000, 0);

      for (uint32_t h = 0; h < settings->n_holes; h++) {
        sprintf(buf, "%u", h + 1);
        vga_text(CENTER_X + 194 + h * 16, 16, buf, 0xffffff, 0, 0);
        sprintf(buf, "%u", settings->scores[1][h]);
        vga_text(
          CENTER_X + 194 + h * 16, 32, h >= nHole ? "-" : buf, 0xffffff, 0, 0
        );
      }
    }

    /*
     * Display game end screen
     */
    if (game_state == GG_GAME_END) {
      uint16_t x0 = CENTER_X - 128, x1 = x0 + 255;
      uint16_t y0 = CENTER_Y - 96, y1 = y0 + 191;

      // Background
      vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_DARK, VGA_ALPHA_BLEND);
      vga_shade(x0, y0, x1, y1, UI_GREEN_DARK, 0);
      vga_frame(x0, y0, x1, y1, 0xffffff, 0);

      // Score counter (common to 1P-2P)
      uint16_t score_x = (VGA_WIDTH - 184) >> 1;
      uint16_t score_y = y0 + 96;
      vga_text(score_x, score_y, " Hole", 0xffffff, 0x000000, 0);

      for (uint32_t h = 0; h < settings->n_holes; h++) {
        sprintf(buf, "%u", h + 1);
        vga_text(score_x + 48 + h * 16, score_y, buf, 0xffffff, 0, 0);
        sprintf(buf, "%u", settings->scores[0][h]);
        vga_text(
          score_x + 48 + h * 16, score_y + 16, h > nHole ? "-" : buf, 0xffffff,
          0, 0
        );
      }

      if (settings->n_players == 1) {
        // Display score screen for singleplayer
        uint32_t score = min(hits[0], 7);
        if (score < 7) sprintf(buf, "%s", score_names[score]);
        else
          sprintf(buf, "%u %s", hits[0] - PAR, score_names[score]);
        uint32_t text_width = strlen(buf) * 12;

        oldfont = vga_font(VGA_FONT_LARGE);
        vga_text((VGA_WIDTH - text_width) >> 1, y0 + 16, buf, 0xffffff, 0, 0);
        vga_font(oldfont);

        sprintf(buf, "%s", score_texts[score]);
        text_width = strlen(buf) * 8;
        vga_text((VGA_WIDTH - text_width) >> 1, y0 + 64, buf, 0xffffff, 0, 0);

        // Single player score
        vga_text(score_x, score_y + 16, "Score", 0xffffff, 0x000000, 0);
      } else if (settings->n_players == 2) {
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
        if (score < 7) sprintf(buf, "%s", score_names[score]);
        else
          sprintf(
            buf, "%u %s", min(hits[0], hits[1]) - PAR, score_names[score]
          );
        uint32_t text_width = strlen(buf) * 8;
        vga_text((VGA_WIDTH - text_width) >> 1, y0 + 64, buf, 0xffffff, 0, 0);

        // Multiplayer scores
        vga_text(score_x, score_y + 16, "   P1", 0xffffff, 0x000000, 0);
        vga_text(score_x, score_y + 32, "   P2", 0xffffff, 0x000000, 0);

        for (uint32_t h = 0; h < settings->n_holes; h++) {
          sprintf(buf, "%u", settings->scores[1][h]);
          vga_text(
            score_x + 48 + h * 16, score_y + 32, h > nHole ? "-" : buf,
            0xffffff, 0, 0
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
      ev = kbd_get_key_event();
      switch (ev.key) {
        case KEY_RETURN:
          // Show scoreboard for last hole
          if (nHole == settings->n_holes - 1) {
            game_state = GG_GAME_SCOREBOARD;
          } else {
            loop = 0;
          }
          break;
      }
    } else if (game_state == GG_GAME_SCOREBOARD) {
      uint16_t x0 = CENTER_X - 192, x1 = x0 + 383;
      uint16_t y0 = CENTER_Y - 96, y1 = y0 + 191;

      // Background
      vga_rect(x0, y0, x1, y1, 0xa0ffffff & UI_GREEN_DARK, VGA_ALPHA_BLEND);
      vga_shade(x0, y0, x1, y1, UI_GREEN_DARK, 0);
      vga_frame(x0, y0, x1, y1, 0xffffff, 0);

      // Score counter (common to 1P-2P)
      uint16_t score_x = (VGA_WIDTH - 232) >> 1;
      uint16_t score_y = y0 + 96;
      vga_text(score_x, score_y, " Hole", 0xffffff, 0x000000, 0);
      vga_text(score_x + 192, score_y, "Total", 0xffffff, 0x000000, 0);

      for (uint32_t h = 0; h < settings->n_holes; h++) {
        sprintf(buf, "%u", h + 1);
        vga_text(score_x + 48 + h * 16, score_y, buf, 0xffffff, 0, 0);
        sprintf(buf, "%u", settings->scores[0][h]);
        vga_text(
          score_x + 48 + h * 16, score_y + 16, h > nHole ? "-" : buf, 0xffffff,
          0, 0
        );
      }

      sprintf(buf, "%u", settings->scores[0][MAX_HOLES]);
      vga_text(score_x + 192, score_y + 16, buf, 0xffffff, 0, 0);

      if (settings->n_players == 1) {
        // Title
        sprintf(buf, "Your score: %u", settings->scores[0][MAX_HOLES]);
        uint32_t text_width = strlen(buf) * 12;

        oldfont = vga_font(VGA_FONT_LARGE);
        vga_text((VGA_WIDTH - text_width) >> 1, y0 + 16, buf, 0xffffff, 0, 0);
        vga_font(oldfont);

        // Single player score
        vga_text(score_x, score_y + 16, "Score", 0xffffff, 0x000000, 0);
      } else if (settings->n_players == 2) {
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
        vga_text(score_x, score_y + 16, "   P1", 0xffffff, 0x000000, 0);
        vga_text(score_x, score_y + 32, "   P2", 0xffffff, 0x000000, 0);

        for (uint32_t h = 0; h < settings->n_holes; h++) {
          sprintf(buf, "%u", settings->scores[1][h]);
          vga_text(
            score_x + 48 + h * 16, score_y + 32, h > nHole ? "-" : buf,
            0xffffff, 0, 0
          );
        }

        sprintf(buf, "%u", settings->scores[1][MAX_HOLES]);
        vga_text(score_x + 192, score_y + 32, buf, 0xffffff, 0, 0);
      }

      // Prompt
      oldfont = vga_font(VGA_FONT_ALT_BOLD);
      vga_text(
        (VGA_WIDTH - 160) >> 1, y1 - 32, "Press RETURN to end", 0xffffff, 0, 0
      );
      vga_font(oldfont);

      // Process input
      ev = kbd_get_key_event();
      switch (ev.key) {
        case KEY_RETURN:
          loop = 0;
          break;
      }
    }

    if (game_state != GG_GAME_RUNNING) {
      // Increment endgame animation timer
      t += ANIM_SPEED * game->frametime;

      // Move camera after endgame
      float t_view = min(t * 0.004f, 1.0f);
      float angle = t * 0.003;
      while (angle > M_PI) angle -= 2.0f * M_PI;
      view_pos.y = lerp(10.0f, 6.0f, t_view);
      view_pos.z = lerp(3.5f, 8.0f, t_view);

      float4x4 view = mat_lookat(view_pos, view_target, view_up);
      view = mmul(view, mat_rotationY(angle));

      gfx_set_matrix(game->ctx, GFX_MAT_VIEW, &view);
    }

    // Draw the frametime counter
    if (game->show_fps) {
      uint64_t fps_times_100 =
        game->frametime == 0 ? 0 : 100000 / game->frametime;
      uint64_t fps = fps_times_100 / 100;
      fps_times_100 %= 100;

      sprintf(
        buf, "Frametime: %llums (%llu.%02llu fps)", game->frametime, fps,
        fps_times_100
      );
      vga_text(0, 0, buf, 0xffffff, 0, VGA_TEXT_BG);
    }

    vga_present();
  }

  return 0;
}

/*
 * Entry point for game
 */
int gg_start_game() {
  // Disable status bar drawing while application is active
  uint8_t is_status_enabled = status_enabled();
  status_set_enabled(0);

  game_t game = {
    .show_fps = 0,
    .frametime = 0,

    .ctx = gfx_create_context(),
  };

  // Initialize background framebuffers
  game.bg_framebuffer = gfx_create_framebuffer(game.ctx, VGA_AUTO, VGA_AUTO);
  game.bg_depthbuffer = gfx_create_depthbuffer(game.ctx, VGA_AUTO, VGA_AUTO);

  // Initialize RNG
  pcg32_random_t rng;
  pcg32_srand(&rng, time(), 1);

  // Make vertices for the hole mesh
  make_hole_mesh();

  // Load the capybara models
  game.pc_base = gfx_load_model(
    obj_capybase, &game.v_base, &game.n_base, &game.vi_base, &game.ni_base
  );
  game.pc_face = gfx_load_model(
    obj_capyface, &game.v_face, &game.n_face, &game.vi_face, &game.ni_face
  );
  game.pc_club = gfx_load_model(
    obj_capyclub, &game.v_club, &game.n_club, &game.vi_club, &game.ni_club
  );
  game.pc_flag = gfx_load_model(
    obj_flag, &game.v_flag, &game.n_flag, &game.vi_flag, &game.ni_flag
  );
  game.pc_pole = gfx_load_model(
    obj_flagpole, &game.v_pole, &game.n_pole, &game.vi_pole, &game.ni_pole
  );
  game.pc_ball = gfx_load_model(
    obj_ball, &game.v_ball, &game.n_ball, &game.vi_ball, &game.ni_ball
  );

  // Init deltatime timer
  game.total_ticks = time();

  // Set "alt" as the default font for the application
  vga_font_t oldfont = vga_font(VGA_FONT_ALT);

  // Main application loop
  while (1) {
    // Display the title screen
    game_settings_t settings;
    settings.n_players = 1;
    settings.n_holes = DEFAULT_HOLES;
    for (int i = 0; i < settings.n_players; i++) {
      settings.scores[i][MAX_HOLES] = 0;
    }

    int play = show_title_screen(&game, &settings);
    if (!play) break;

    for (uint32_t hole = 0; hole < settings.n_holes; hole++) {
      int quit = play_game(&game, &settings, hole, &rng);
      if (quit) break;
    }
  }

  // Free framebuffers
  mem_free(game.bg_framebuffer);
  mem_free(game.bg_depthbuffer);

  // Restore font
  vga_font(oldfont);

  // Restore status bar enabled state
  status_set_enabled(is_status_enabled);

  return 0;
}
