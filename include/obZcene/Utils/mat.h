#ifndef OBZ_UTILS_MAT_H
#define OBZ_UTILS_MAT_H

#include "vec.h"
#include <assert.h>
#include <math.h>
#include <string.h>

/* --- small Vec4 helper --- */
typedef struct
{
  float x, y, z, w;
} Vec4;
inline static Vec4 obz_vec4(float x, float y, float z, float w) { return (Vec4){x, y, z, w}; }

/*
  Layout: row-major
  element (r,c) at m[r*cols + c]
  Mat3: m[9]  : rows 0..2, cols 0..2
  Mat4: m[16] : rows 0..3, cols 0..3
*/

typedef struct
{
  float m[9];
} Mat3;
typedef struct
{
  float m[16];
} Mat4;

/* -------------------- Mat3 -------------------- */
inline static Mat3 obz_mat3_identity(void)
{
  /* Identity matrix: diag = 1, others 0 */
  /* I = [[1,0,0],[0,1,0],[0,0,1]] */
  Mat3 M;
  memset(M.m, 0, sizeof M.m);
  M.m[0 * 3 + 0] = 1.0f;
  M.m[1 * 3 + 1] = 1.0f;
  M.m[2 * 3 + 2] = 1.0f;
  return M;
}

inline static Mat3 obz_mat3_from_cols(Vec3 c0, Vec3 c1, Vec3 c2)
{
  /* Form matrix from column vectors: M = [c0 c1 c2] */
  Mat3 M;
  M.m[0] = c0.x;
  M.m[1] = c1.x;
  M.m[2] = c2.x;
  M.m[3] = c0.y;
  M.m[4] = c1.y;
  M.m[5] = c2.y;
  M.m[6] = c0.z;
  M.m[7] = c1.z;
  M.m[8] = c2.z;
  return M;
}

inline static Mat3 obz_mat3_mul(const Mat3* a, const Mat3* b)
{
  /* Matrix multiply: r_ij = sum_k a_ik * b_kj */
  Mat3 r;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      float s = 0.0f;
      for (int k = 0; k < 3; ++k)
        s += a->m[i * 3 + k] * b->m[k * 3 + j];
      r.m[i * 3 + j] = s;
    }
  }
  return r;
}

inline static Vec3 obz_mat3_mul_vec3(const Mat3* a, Vec3 v)
{
  /* Matrix * vector: each row dot vector */
  return (Vec3){a->m[0] * v.x + a->m[1] * v.y + a->m[2] * v.z,
                a->m[3] * v.x + a->m[4] * v.y + a->m[5] * v.z,
                a->m[6] * v.x + a->m[7] * v.y + a->m[8] * v.z};
}

inline static Mat3 obz_mat3_transpose(const Mat3* a)
{
  /* Swap rows/columns: r_ij = a_ji */
  Mat3 r;
  r.m[0] = a->m[0];
  r.m[1] = a->m[3];
  r.m[2] = a->m[6];
  r.m[3] = a->m[1];
  r.m[4] = a->m[4];
  r.m[5] = a->m[7];
  r.m[6] = a->m[2];
  r.m[7] = a->m[5];
  r.m[8] = a->m[8];
  return r;
}

/* rotation matrix from axis (must be normalized) and angle (rad) */
inline static Mat3 obz_mat3_from_axis_angle(Vec3 axis, float rad)
{
  /* Rodrigues formula: R = cI + t aa^T + s [a]_x */
  float c = cosf(rad), s = sinf(rad);
  float t = 1.0f - c;
  Vec3  n = obz_vec3_norm(axis);
  float x = n.x, y = n.y, z = n.z;
  Mat3  R;
  R.m[0] = t * x * x + c;
  R.m[1] = t * x * y - s * z;
  R.m[2] = t * x * z + s * y;
  R.m[3] = t * x * y + s * z;
  R.m[4] = t * y * y + c;
  R.m[5] = t * y * z - s * x;
  R.m[6] = t * x * z - s * y;
  R.m[7] = t * y * z + s * x;
  R.m[8] = t * z * z + c;
  return R;
}

/* -------------------- Mat4 -------------------- */
inline static Mat4 obz_mat4_identity(void)
{
  /* 4x4 identity */
  Mat4 M;
  memset(M.m, 0, sizeof M.m);
  M.m[0 * 4 + 0] = 1.0f;
  M.m[1 * 4 + 1] = 1.0f;
  M.m[2 * 4 + 2] = 1.0f;
  M.m[3 * 4 + 3] = 1.0f;
  return M;
}

/* multiply: r = a * b (row-major) */
inline static Mat4 obz_mat4_mul(const Mat4* a, const Mat4* b)
{
  /* Standard matrix multiplication: r_ij = sum_k a_ik b_kj */
  Mat4 r;
  for (int i = 0; i < 4; ++i)
  {
    const int row = i * 4;
    for (int j = 0; j < 4; ++j)
    {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k)
        s += a->m[row + k] * b->m[k * 4 + j];
      r.m[row + j] = s;
    }
  }
  return r;
}

/* transform point (assumes w=1), returns Vec3 after perspective divide if w != 1 */
inline static Vec3 obz_mat4_transform_point(const Mat4* m, Vec3 p)
{
  /* Compute (x,y,z,w) = M * (p.x,p.y,p.z,1). Apply perspective divide if needed. */
  float x = m->m[0] * p.x + m->m[1] * p.y + m->m[2] * p.z + m->m[3];
  float y = m->m[4] * p.x + m->m[5] * p.y + m->m[6] * p.z + m->m[7];
  float z = m->m[8] * p.x + m->m[9] * p.y + m->m[10] * p.z + m->m[11];
  float w = m->m[12] * p.x + m->m[13] * p.y + m->m[14] * p.z + m->m[15];
  if (w != 0.0f && w != 1.0f)
  {
    float iw = 1.0f / w;
    return (Vec3){x * iw, y * iw, z * iw};
  }
  return (Vec3){x, y, z};
}

/* transform direction (assumes w=0) - ignores translation */
inline static Vec3 obz_mat4_transform_dir(const Mat4* m, Vec3 d)
{
  /* Only the upper-left 3x3 affects direction vectors */
  return (Vec3){m->m[0] * d.x + m->m[1] * d.y + m->m[2] * d.z,
                m->m[4] * d.x + m->m[5] * d.y + m->m[6] * d.z,
                m->m[8] * d.x + m->m[9] * d.y + m->m[10] * d.z};
}

/* translation/scale/rotate constructors */
inline static Mat4 obz_mat4_translate(float tx, float ty, float tz)
{
  /* Translation matrix: last column = (tx,ty,tz,1) */
  Mat4 M  = obz_mat4_identity();
  M.m[3]  = tx;
  M.m[7]  = ty;
  M.m[11] = tz;
  return M;
}

inline static Mat4 obz_mat4_scale(float sx, float sy, float sz)
{
  /* Scaling matrix: diagonal = (sx,sy,sz,1) */
  Mat4 M;
  memset(M.m, 0, sizeof M.m);
  M.m[0]  = sx;
  M.m[5]  = sy;
  M.m[10] = sz;
  M.m[15] = 1.0f;
  return M;
}

inline static Mat4 obz_mat4_rotate_x(float rad)
{
  /* Rotation about X-axis */
  float c = cosf(rad), s = sinf(rad);
  Mat4  M = obz_mat4_identity();
  M.m[5]  = c;
  M.m[6]  = -s;
  M.m[9]  = s;
  M.m[10] = c;
  return M;
}
inline static Mat4 obz_mat4_rotate_y(float rad)
{
  /* Rotation about Y-axis */
  float c = cosf(rad), s = sinf(rad);
  Mat4  M = obz_mat4_identity();
  M.m[0]  = c;
  M.m[2]  = s;
  M.m[8]  = -s;
  M.m[10] = c;
  return M;
}
inline static Mat4 obz_mat4_rotate_z(float rad)
{
  /* Rotation about Z-axis */
  float c = cosf(rad), s = sinf(rad);
  Mat4  M = obz_mat4_identity();
  M.m[0]  = c;
  M.m[1]  = -s;
  M.m[4]  = s;
  M.m[5]  = c;
  return M;
}

/* rotation around arbitrary axis (normalized) */
inline static Mat4 obz_mat4_rotate_axis(Vec3 axis, float rad)
{
  /* Embed 3x3 axis-angle rotation into 4x4 */
  Mat3 r3 = obz_mat3_from_axis_angle(axis, rad);
  Mat4 M  = obz_mat4_identity();
  M.m[0]  = r3.m[0];
  M.m[1]  = r3.m[1];
  M.m[2]  = r3.m[2];
  M.m[4]  = r3.m[3];
  M.m[5]  = r3.m[4];
  M.m[6]  = r3.m[5];
  M.m[8]  = r3.m[6];
  M.m[9]  = r3.m[7];
  M.m[10] = r3.m[8];
  return M;
}

/* perspective projection (right-handed, clip z in [-1,1]) */
inline static Mat4 obz_mat4_perspective(float fovy_rad, float aspect, float znear, float zfar)
{
  /* Standard perspective matrix */
  float f = 1.0f / tanf(fovy_rad * 0.5f);
  Mat4  M;
  memset(M.m, 0, sizeof M.m);
  M.m[0]  = f / aspect;
  M.m[5]  = f;
  M.m[10] = (zfar + znear) / (znear - zfar);
  M.m[11] = (2.0f * zfar * znear) / (znear - zfar);
  M.m[14] = -1.0f;
  return M;
}

/* orthographic projection */
inline static Mat4 obz_mat4_ortho(float left, float right, float bottom, float top, float zn,
                                  float zf)
{
  /* Maps axis-aligned box to clip space
    Formula:
    M00 = 2/(r-l)
    M11 = 2/(t-b)
    M22 = -2/(zf-zn)
    M03 = -(r+l)/(r-l)
    M13 = -(t+b)/(t-b)
    M23 = -(zf+zn)/(zf-zn)
    */
  Mat4 M;
  memset(M.m, 0, sizeof M.m);
  M.m[0]  = 2.0f / (right - left);
  M.m[5]  = 2.0f / (top - bottom);
  M.m[10] = -2.0f / (zf - zn);
  M.m[3]  = -(right + left) / (right - left);
  M.m[7]  = -(top + bottom) / (top - bottom);
  M.m[11] = -(zf + zn) / (zf - zn);
  M.m[15] = 1.0f;
  return M;
}

/* transpose */
inline static Mat4 obz_mat4_transpose(const Mat4* a)
{
  /* r_ij = a_ji */
  Mat4 r;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      r.m[i * 4 + j] = a->m[j * 4 + i];
  return r;
}

/*
Fast affine inverse: assumes bottom row is [0 0 0 1] (common for model/world transforms).
Inverts the upper-left 3x3 (rotation+scale/shear) and computes inverse translation.
*/
inline static Mat4 obz_mat4_invert_affine(const Mat4* a)
{
  Mat4 r;
  /* extract 3x3 and transpose (if rotation-only, inverse == transpose) */
  /* compute inverse of 3x3 by adjugate/determinant (general) */
  float A00 = a->m[0], A01 = a->m[1], A02 = a->m[2];
  float A10 = a->m[4], A11 = a->m[5], A12 = a->m[6];
  float A20 = a->m[8], A21 = a->m[9], A22 = a->m[10];

  /* compute cofactors */
  float co00 = A11 * A22 - A12 * A21;
  float co01 = -(A10 * A22 - A12 * A20);
  float co02 = A10 * A21 - A11 * A20;
  float co10 = -(A01 * A22 - A02 * A21);
  float co11 = A00 * A22 - A02 * A20;
  float co12 = -(A00 * A21 - A01 * A20);
  float co20 = A01 * A12 - A02 * A11;
  float co21 = -(A00 * A12 - A02 * A10);
  float co22 = A00 * A11 - A01 * A10;

  float det = A00 * co00 + A01 * co01 + A02 * co02;
  if (det == 0.0f)
  {
    /* non-invertible; return identity as safe fallback */
    return obz_mat4_identity();
  }
  float idet = 1.0f / det;

  /* inverse 3x3 = (1/det) * adjugate(cofactor matrix transposed) */
  r.m[0]  = co00 * idet;
  r.m[1]  = co10 * idet;
  r.m[2]  = co20 * idet;
  r.m[3]  = 0.0f;
  r.m[4]  = co01 * idet;
  r.m[5]  = co11 * idet;
  r.m[6]  = co21 * idet;
  r.m[7]  = 0.0f;
  r.m[8]  = co02 * idet;
  r.m[9]  = co12 * idet;
  r.m[10] = co22 * idet;
  r.m[11] = 0.0f;

  /* inverse translation: -R^{-1} * t (where t = a[3,7,11]) */
  float tx = a->m[3], ty = a->m[7], tz = a->m[11];
  r.m[3]  = -(r.m[0] * tx + r.m[1] * ty + r.m[2] * tz);
  r.m[7]  = -(r.m[4] * tx + r.m[5] * ty + r.m[6] * tz);
  r.m[11] = -(r.m[8] * tx + r.m[9] * ty + r.m[10] * tz);

  r.m[12] = 0.0f;
  r.m[13] = 0.0f;
  r.m[14] = 0.0f;
  r.m[15] = 1.0f;
  return r;
}

#endif
