#ifndef FPMATH_H
#define FPMATH_H

typedef struct {
  float x, y, z;
} float3;

typedef struct {
  float x, y, z, w;
} float4;

typedef struct {
  float v[4][4];
} float4x4;

/* Vector functions */

float vdot(float3 a, float3 b);

float3 vcross(float3 a, float3 b);

/* Vector operations */

float3 vadd(float3 a, float3 b);

float3 vadds(float3 a, float s);

float3 vsub(float3 a, float3 b);

float3 vsubs(float3 a, float s);

float3 vmul(float3 a, float3 b);

float3 vmuls(float3 a, float s);

float3 vdiv(float3 a, float3 b);

float3 vdivs(float3 a, float s);

/* Vector conversions */

/*
 * Extend a float3 to float4
 */
float4 vext(float3 v, float w);

/*
 * Reduce a float4 to float3
 */
float3 vred(float4 v);

/*
 * Perspective project a float4 to float3
 */
float3 vpersp(float4 v);

/* Vec4 operations */

float4 v4add(float4 a, float4 b);

float4 v4adds(float4 a, float s);

float4 v4sub(float4 a, float4 b);

float4 v4subs(float4 a, float s);

float4 v4mul(float4 a, float4 b);

float4 v4muls(float4 a, float s);

float4 v4div(float4 a, float4 b);

float4 v4divs(float4 a, float s);

/* Matrix operations */

float4x4 madd(float4x4 a, float4x4 b);

float4x4 msub(float4x4 a, float4x4 b);

float4x4 mmul(float4x4 a, float4x4 b);

void mtrans(float4x4 *m);

/* Matrix-vector operations */

float4 mvmul(float4x4 m, float4 v);

#endif
