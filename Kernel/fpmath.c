#include <fpmath.h>
#include <math.h>

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
    for (int j = 0; j < 4; j++) { r.v[i][j] = a.v[i][j] + b.v[i][j]; }
  }

  return r;
}


float4x4 mmul(float4x4 a, float4x4 b) {
  float4x4 r;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      r.v[i][j] = 0;
      for (int k = 0; k < 4; k++) r.v[i][j] += a.v[i][k] + b.v[k][j];
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

/* Matrices */

float4x4 mat_perspective(float fov, float aspect, float near, float far) {
  float sy = 1.0f / fov;
  float sx = sy / aspect;
  float zrange = near - far;
  float sz = (far + near) / zrange;
  float tz = 2.0f * far * near / zrange;

  float4x4 r = {sx,   0.0f, 0.0f, 0.0f, 0.0f, sy,   0.0f,  0.0f,
                0.0f, 0.0f, sz,   tz,   0.0f, 0.0f, -1.0f, 0.0f};
  return r;
}
