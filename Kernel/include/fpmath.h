#ifndef FPMATH_H
#define FPMATH_H

#define M_PI 3.14159265358979323846264338327950288
#define M_INVPI 0.31830988618379067153776752674502872

#define fabs(x) ((x) > 0 ? (x) : -(x))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

typedef struct {
  float x, y, z;
} float3;

typedef struct {
  float x, y, z, w;
} float4;

typedef struct {
  float v[4][4];
} float4x4;

/* Floating point math functions */
extern float sqrt(float x);

float sin(float x);

float cos(float x);

float tan(float x);

/* Vector functions */

float vdot(float3 a, float3 b);

float3 vcross(float3 a, float3 b);

float vabs(float3 v);

float3 vnorm(float3 v);

/* Vector operations */

float3 vadd(float3 a, float3 b);

float3 vadds(float3 a, float s);

float3 vsub(float3 a, float3 b);

float3 vsubs(float3 a, float s);

float3 vmul(float3 a, float3 b);

float3 vmuls(float3 a, float s);

float3 vdiv(float3 a, float3 b);

float3 vdivs(float3 a, float s);

/*
 * Clamp the ocmponents of a vector between 0 and 1
 */
float3 vsat(float3 v);

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

/*
 * Transpose a matrix in place.
 */
void mtrans(float4x4 *m);

/*
 * Calculate the transposed adjoint of a matrix, aka cofactor matrix.
 * This function treats the input, and output, as 3x3 matrices. 4x4 matrix
 * type is used for convenience only.
 */
float4x4 mtadj(float4x4 m);

/* Matrix-vector operations */

/*
 * Multiply a 4x4 matrix by a 4-vector.
 */
float4 mvmul(float4x4 m, float4 v);

/*
 * Multiply a 3x3 matrix by a 3-vector.
 * The matrix input is treated as 3x3; the last column and row are ignored.
 */
float3 mvmul3(float4x4 m, float3 v);

/* Matrices */

float4x4 mat_perspective(float fov, float aspect, float near, float far);

float4x4 mat_translation(float x, float y, float z);

float4x4 mat_rotation(float angle, float3 rotationAxis);

float4x4 mat_rotationX(float angle);
float4x4 mat_rotationY(float angle);
float4x4 mat_rotationZ(float angle);

float4x4 mat_scale(float x, float y, float z);

float4x4 mat_lookat(float3 pos, float3 target, float3 up);

#endif
