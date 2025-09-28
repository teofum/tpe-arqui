#include <fpmath.h>

float fit_to_pi(float x) {
  while (x > M_PI) { x -= 2.0f * M_PI; }
  while (x < -M_PI) { x += 2.0f * M_PI; }

  return x;
}

float sin(float x) {
  const float b = 4.0f * M_INVPI;
  const float c = -4.0f * M_INVPI * M_INVPI;

  float y = b * x + c * x * fabs(x);

  const float p = 0.225;
  y = p * (y * fabs(y) - y) + y;

  return y;
}

float cos(float x) {
  const float b = 4.0f * M_INVPI;
  const float c = -4.0f * M_INVPI * M_INVPI;

  x += 0.5f * M_PI;
  if (x > M_PI) x -= 2.0f * M_PI;

  float y = b * x + c * x * fabs(x);

  const float p = 0.225;
  y = p * (y * fabs(y) - y) + y;

  return y;
}

float tan(float x) { return sin(x) / cos(x); }

/* Vector functions */

float vdot(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

float3 vcross(float3 a, float3 b) {
  float3 r = {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x,
  };
  return r;
}

float vabs(float3 v) { return sqrt(vdot(v, v)); }

float vabssq(float3 v) { return vdot(v, v); }

float3 vnorm(float3 v) { return vdivs(v, vabs(v)); }

/* Vector operations */

float3 vadd(float3 a, float3 b) {
  float3 r = {a.x + b.x, a.y + b.y, a.z + b.z};
  return r;
}

float3 vadds(float3 a, float s) {
  float3 r = {a.x + s, a.y + s, a.z + s};
  return r;
}

float3 vsub(float3 a, float3 b) {
  float3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
  return r;
}

float3 vsubs(float3 a, float s) {
  float3 r = {a.x - s, a.y - s, a.z - s};
  return r;
}

float3 vmul(float3 a, float3 b) {
  float3 r = {a.x * b.x, a.y * b.y, a.z * b.z};
  return r;
}

float3 vmuls(float3 a, float s) {
  float3 r = {a.x * s, a.y * s, a.z * s};
  return r;
}

float3 vdiv(float3 a, float3 b) {
  float3 r = {a.x / b.x, a.y / b.y, a.z / b.z};
  return r;
}

float3 vdivs(float3 a, float s) {
  float3 r = {a.x / s, a.y / s, a.z / s};
  return r;
}

float3 vsat(float3 v) {
  float3 r = {
    min(1.0f, max(0.0f, v.x)),
    min(1.0f, max(0.0f, v.y)),
    min(1.0f, max(0.0f, v.z)),
  };
  return r;
}

/* Vector conversions */

/*
 * Extend a float3 to float4
 */
float4 vext(float3 v, float w) {
  float4 r = {v.x, v.y, v.z, w};
  return r;
}

/*
 * Reduce a float4 to float3
 */
float3 vred(float4 v) {
  float3 r = {v.x, v.y, v.z};
  return r;
}

/*
 * Perspective project a float4 to float3
 */
float3 vpersp(float4 v) {
  float3 r = {v.x / v.w, v.y / v.w, v.z / v.w};
  return r;
}

/* Vec4 operations */

float4 v4add(float4 a, float4 b) {
  float4 r = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
  return r;
}

float4 v4adds(float4 a, float s) {
  float4 r = {a.x + s, a.y + s, a.z + s, a.w + s};
  return r;
}

float4 v4sub(float4 a, float4 b) {
  float4 r = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
  return r;
}

float4 v4subs(float4 a, float s) {
  float4 r = {a.x - s, a.y - s, a.z - s, a.w - s};
  return r;
}

float4 v4mul(float4 a, float4 b) {
  float4 r = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
  return r;
}

float4 v4muls(float4 a, float s) {
  float4 r = {a.x * s, a.y * s, a.z * s, a.w * s};
  return r;
}

float4 v4div(float4 a, float4 b) {
  float4 r = {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
  return r;
}

float4 v4divs(float4 a, float s) {
  float4 r = {a.x / s, a.y / s, a.z / s, a.w / s};
  return r;
}

/* Matrix operations */

float4x4 madd(float4x4 a, float4x4 b) {
  float4x4 r;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) { r.v[i][j] = a.v[i][j] + b.v[i][j]; }
  }

  return r;
}

float4x4 msub(float4x4 a, float4x4 b) {
  float4x4 r;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) { r.v[i][j] = a.v[i][j] - b.v[i][j]; }
  }

  return r;
}


float4x4 mmul(float4x4 a, float4x4 b) {
  float4x4 r;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      r.v[i][j] = 0;
      for (int k = 0; k < 4; k++) r.v[i][j] += a.v[i][k] * b.v[k][j];
    }
  }

  return r;
}

void mtrans(float4x4 *m) {
  float temp;
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      temp = m->v[i][j];
      m->v[i][j] = m->v[j][i];
      m->v[j][i] = temp;
    }
  }
}

#define det2x2(a, b, c, d) ((a) * (d) - (b) * (c))

float4x4 mtadj(float4x4 m) {
  float4x4 c = {
    det2x2(m.v[1][1], m.v[1][2], m.v[2][1], m.v[2][2]),
    det2x2(m.v[1][0], m.v[1][2], m.v[2][0], m.v[2][2]),
    det2x2(m.v[1][0], m.v[1][1], m.v[2][0], m.v[2][1]),
    0,
    det2x2(m.v[0][1], m.v[0][2], m.v[2][1], m.v[2][2]),
    det2x2(m.v[0][0], m.v[0][2], m.v[2][0], m.v[2][2]),
    det2x2(m.v[0][0], m.v[0][1], m.v[2][0], m.v[2][1]),
    0,
    det2x2(m.v[0][1], m.v[0][2], m.v[1][1], m.v[1][2]),
    det2x2(m.v[0][0], m.v[0][2], m.v[1][0], m.v[1][2]),
    det2x2(m.v[0][0], m.v[0][1], m.v[1][0], m.v[1][1]),
    0,
    0,
    0,
    0,
    1,
  };
  return c;
}

/* Matrix-vector operations */

float4 mvmul(float4x4 m, float4 v) {
  float4 r = {
    v.x * m.v[0][0] + v.y * m.v[0][1] + v.z * m.v[0][2] + v.w * m.v[0][3],
    v.x * m.v[1][0] + v.y * m.v[1][1] + v.z * m.v[1][2] + v.w * m.v[1][3],
    v.x * m.v[2][0] + v.y * m.v[2][1] + v.z * m.v[2][2] + v.w * m.v[2][3],
    v.x * m.v[3][0] + v.y * m.v[3][1] + v.z * m.v[3][2] + v.w * m.v[3][3],
  };
  return r;
}

float3 mvmul3(float4x4 m, float3 v) {
  float3 r = {
    v.x * m.v[0][0] + v.y * m.v[0][1] + v.z * m.v[0][2],
    v.x * m.v[1][0] + v.y * m.v[1][1] + v.z * m.v[1][2],
    v.x * m.v[2][0] + v.y * m.v[2][1] + v.z * m.v[2][2],
  };
  return r;
}


/* Matrices */

float4x4 mat_perspective(float fov, float aspect, float near, float far) {
  float sy = 1.0f / tan(fov * 0.5f);
  float sx = sy / aspect;
  float zrange = near - far;
  float sz = (far + near) / zrange;
  float tz = 2.0f * far * near / zrange;

  float4x4 r = {sx,   0.0f, 0.0f, 0.0f, 0.0f, sy,   0.0f,  0.0f,
                0.0f, 0.0f, sz,   tz,   0.0f, 0.0f, -1.0f, 0.0f};
  return r;
}

float4x4 mat_translation(float x, float y, float z) {
  float4x4 r = {1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1};
  return r;
}

float4x4 mat_rotation(float angle, float3 rotationAxis) {
  float c = cos(angle);
  float s = sin(angle);

  float3 axis = vnorm(rotationAxis);
  float3 temp = vmuls(axis, 1.0f - c);

  float4x4 r = {
    c + temp.x * axis.x,
    temp.y * axis.x - s * axis.z,
    temp.z * axis.x + s * axis.y,
    0,
    temp.x * axis.y + s * axis.z,
    c + temp.y * axis.y,
    temp.z * axis.y - s * axis.x,
    0,
    temp.x * axis.z - s * axis.y,
    temp.y * axis.z + s * axis.x,
    c + temp.z * axis.z,
    0,
    0,
    0,
    0,
    1,
  };
  return r;
}

float4x4 mat_rotationX(float angle) {
  float c = cos(angle);
  float s = sin(angle);

  float4x4 r = {1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1};
  return r;
}

float4x4 mat_rotationY(float angle) {
  float c = cos(angle);
  float s = sin(angle);

  float4x4 r = {c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1};
  return r;
}

float4x4 mat_rotationZ(float angle) {
  float c = cos(angle);
  float s = sin(angle);

  float4x4 r = {c, 0, -s, 0, 0, s, c, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  return r;
}

float4x4 mat_scale(float x, float y, float z) {
  float4x4 r = {x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1};
  return r;
}

float4x4 mat_lookat(float3 pos, float3 target, float3 up) {
  float3 delta = vsub(pos, target);
  if (vabs(delta) < 0.001f) return mat_scale(1, 1, 1);

  float3 f = vnorm(delta);
  float3 s = vnorm(vcross(up, f));
  float3 u = vcross(f, s);

  float4x4 r = {
    s.x, s.y, s.z, -vdot(s, pos), u.x, u.y, u.z, -vdot(u, pos),
    f.x, f.y, f.z, -vdot(f, pos), 0,   0,   0,   1,
  };
  return r;
}
