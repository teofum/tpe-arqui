#ifndef FPMATH_H
#define FPMATH_H

#define M_PI 3.14159265358979323846264338327950288
#define M_INVPI 0.31830988618379067153776752674502872

#define fabs(x) ((x) > 0 ? (x) : -(x))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

/*
 * Three component vector.
 */
typedef struct {
  float x, y, z;
} float3;

/*
 * Four component vector, because sometimes you just need an extra dimension.
 */
typedef struct {
  float x, y, z, w;
} float4;

/*
 * 4x4 matrix. Stored in row-major ordering.
 */
typedef struct {
  float v[4][4];
} float4x4;

/* Floating point math functions */

/*
 * Square root
 */
extern float sqrt(float x);

/*
 * Fits an angle to [-pi;+pi]
 */
float fit_to_pi(float x);

/*
 * Fast sine approximation, input in radians.
 * Input must be between -PI and +PI.
 */
float sin(float x);

/*
 * Fast cosine approximation, input in radians.
 * Input must be between -PI and +PI.
 */
float cos(float x);

/*
 * Tangent approximation.
 * Input must be between -PI and +PI.
 */
float tan(float x);

/* Vector functions */

/* Dot product */
float vdot(float3 a, float3 b);

/* Cross product */
float3 vcross(float3 a, float3 b);

/* Magnitude (norm) of a vector */
float vabs(float3 v);

/* Squared magnitude (norm) of a vector */
float vabssq(float3 v);

/* Normalized vector */
float3 vnorm(float3 v);

/* Vector operations */

/* Vector addition */
float3 vadd(float3 a, float3 b);

/* Vector-scalar addition */
float3 vadds(float3 a, float s);

/* Vector subtraction */
float3 vsub(float3 a, float3 b);

/* Vector-scalar subtraction */
float3 vsubs(float3 a, float s);

/* Vector multiplication (component wise) */
float3 vmul(float3 a, float3 b);

/* Vector-scalar multiplication */
float3 vmuls(float3 a, float s);

/* Vector division (component wise) */
float3 vdiv(float3 a, float3 b);

/* Vector-scalar division */
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

/* 4D vector addition */
float4 v4add(float4 a, float4 b);

/* 4D vector-scalar addition */
float4 v4adds(float4 a, float s);

/* 4D vector subtraction */
float4 v4sub(float4 a, float4 b);

/* 4D vector-scalar subtraction */
float4 v4subs(float4 a, float s);

/* 4D vector multiplication (component wise) */
float4 v4mul(float4 a, float4 b);

/* 4D vector-scalar multiplication */
float4 v4muls(float4 a, float s);

/* 4D vector division (component wise) */
float4 v4div(float4 a, float4 b);

/* 4D vector-scalar division */
float4 v4divs(float4 a, float s);

/* Matrix operations */

/* Matrix addition */
float4x4 madd(float4x4 a, float4x4 b);

/* Matrix subtraction */
float4x4 msub(float4x4 a, float4x4 b);

/* Matrix multiplication */
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

/*
 * Creates a perspective projection matrix with the given (vertical) FOV and
 * aspect ratio. Takes a near and far plane distance.
 */
float4x4 mat_perspective(float fov, float aspect, float near, float far);

/*
 * Creates a translation matrix.
 */
float4x4 mat_translation(float x, float y, float z);

/*
 * Creates a rotation matrix with an axis-angle approach.
 * This method is slower, for axis-aligned rotations use the specialized
 * functions.
 */
float4x4 mat_rotation(float angle, float3 rotationAxis);

/* Creates a rotation matrix, rotating along the X axis. */
float4x4 mat_rotationX(float angle);

/* Creates a rotation matrix, rotating along the Y axis. */
float4x4 mat_rotationY(float angle);

/* Creates a rotation matrix, rotating along the Z axis. */
float4x4 mat_rotationZ(float angle);

/*
 * Creates a scaling matrix.
 */
float4x4 mat_scale(float x, float y, float z);

/*
 * Creates a transform matrix for a given direction, position and "up" vector.
 * Very useful for view matrices.
 */
float4x4 mat_lookat(float3 pos, float3 target, float3 up);

#endif
