#ifndef OBZ_MATH_VEC_H
#define OBZ_MATH_VEC_H

#include "obz_macros.h"

OBZ_BEGIN_CPP_DECLS

/*-------------------------------------------------------
    Vector Types
-------------------------------------------------------*/
typedef struct
{
  float x, y;
} Vec2f;
typedef struct
{
  float x, y, z;
} Vec3f;

typedef struct
{
  double x, y;
} Vec2d;

typedef struct
{
  double x, y, z;
} Vec3d;

typedef struct
{
  float x, y, z, w;
} Vec4f;

typedef struct
{
  double x, y, z, w;
} Vec4d;

/* Constructors */
OBZ_API_INLINE static Vec2f obz_vec2f(float x, float y) { return (Vec2f){x, y}; }
OBZ_API_INLINE static Vec3f obz_vec3f(float x, float y, float z) { return (Vec3f){x, y, z}; }

OBZ_API_INLINE static Vec2d obz_vec2d(double x, double y) { return (Vec2d){x, y}; }
OBZ_API_INLINE static Vec3d obz_vec3d(double x, double y, double z) { return (Vec3d){x, y, z}; }

OBZ_API_INLINE static Vec4f obz_vec4f(float x, float y, float z, float w)
{
  return (Vec4f){x, y, z, w};
}

OBZ_API_INLINE static Vec4d obz_vec4d(double x, double y, double z, double w)
{
  return (Vec4d){x, y, z, w};
}

/*-------------------------------------------------------
    Vec2f API
-------------------------------------------------------*/
OBZ_API Vec2f obz_vec2f_add(Vec2f a, Vec2f b);
OBZ_API Vec2f obz_vec2f_sub(Vec2f a, Vec2f b);
OBZ_API Vec2f obz_vec2f_mul(Vec2f a, Vec2f b);
OBZ_API Vec2f obz_vec2f_div(Vec2f a, Vec2f b);

OBZ_API Vec2f obz_vec2f_addf(Vec2f v, float s);
OBZ_API Vec2f obz_vec2f_subf(Vec2f v, float s);
OBZ_API Vec2f obz_vec2f_mulf(Vec2f v, float s);
OBZ_API Vec2f obz_vec2f_divf(Vec2f v, float s);

OBZ_API float obz_vec2f_dot(Vec2f a, Vec2f b);
OBZ_API float obz_vec2f_len(Vec2f v);
OBZ_API Vec2f obz_vec2f_norm(Vec2f v);
OBZ_API Vec2f obz_vec2f_lerp(Vec2f a, Vec2f b, float t);
OBZ_API Vec2f obz_vec2f_rotate(Vec2f v, float rad);

/*-------------------------------------------------------
    Vec2d API
-------------------------------------------------------*/
OBZ_API Vec2d obz_vec2d_add(Vec2d a, Vec2d b);
OBZ_API Vec2d obz_vec2d_sub(Vec2d a, Vec2d b);
OBZ_API Vec2d obz_vec2d_mul(Vec2d a, Vec2d b);
OBZ_API Vec2d obz_vec2d_div(Vec2d a, Vec2d b);

OBZ_API Vec2d obz_vec2d_addf(Vec2d v, double s);
OBZ_API Vec2d obz_vec2d_subf(Vec2d v, double s);
OBZ_API Vec2d obz_vec2d_mulf(Vec2d v, double s);
OBZ_API Vec2d obz_vec2d_divf(Vec2d v, double s);

OBZ_API double obz_vec2d_dot(Vec2d a, Vec2d b);
OBZ_API double obz_vec2d_len(Vec2d v);
OBZ_API Vec2d  obz_vec2d_norm(Vec2d v);
OBZ_API Vec2d  obz_vec2d_lerp(Vec2d a, Vec2d b, double t);
OBZ_API Vec2d  obz_vec2d_rotate(Vec2d v, double rad);

/*-------------------------------------------------------
    Vec3f API
-------------------------------------------------------*/
OBZ_API Vec3f obz_vec3f_add(Vec3f a, Vec3f b);
OBZ_API Vec3f obz_vec3f_sub(Vec3f a, Vec3f b);
OBZ_API Vec3f obz_vec3f_mul(Vec3f a, Vec3f b);
OBZ_API Vec3f obz_vec3f_div(Vec3f a, Vec3f b);

OBZ_API Vec3f obz_vec3f_addf(Vec3f v, float s);
OBZ_API Vec3f obz_vec3f_subf(Vec3f v, float s);
OBZ_API Vec3f obz_vec3f_mulf(Vec3f v, float s);
OBZ_API Vec3f obz_vec3f_divf(Vec3f v, float s);

OBZ_API float obz_vec3f_dot(Vec3f a, Vec3f b);
OBZ_API Vec3f obz_vec3f_cross(Vec3f a, Vec3f b);

OBZ_API Vec3f obz_vec3f_rotate_x(Vec3f v, float rad);
OBZ_API Vec3f obz_vec3f_rotate_y(Vec3f v, float rad);
OBZ_API Vec3f obz_vec3f_rotate_z(Vec3f v, float rad);

OBZ_API float obz_vec3f_len(Vec3f v);
OBZ_API Vec3f obz_vec3f_norm(Vec3f v);
OBZ_API Vec3f obz_vec3f_lerp(Vec3f a, Vec3f b, float t);

/*-------------------------------------------------------
    Vec3d API
-------------------------------------------------------*/
OBZ_API Vec3d obz_vec3d_add(Vec3d a, Vec3d b);
OBZ_API Vec3d obz_vec3d_sub(Vec3d a, Vec3d b);
OBZ_API Vec3d obz_vec3d_mul(Vec3d a, Vec3d b);
OBZ_API Vec3d obz_vec3d_div(Vec3d a, Vec3d b);

OBZ_API Vec3d obz_vec3d_addf(Vec3d v, double s);
OBZ_API Vec3d obz_vec3d_subf(Vec3d v, double s);
OBZ_API Vec3d obz_vec3d_mulf(Vec3d v, double s);
OBZ_API Vec3d obz_vec3d_divf(Vec3d v, double s);

OBZ_API double obz_vec3d_dot(Vec3d a, Vec3d b);
OBZ_API Vec3d  obz_vec3d_cross(Vec3d a, Vec3d b);

OBZ_API Vec3d obz_vec3d_rotate_x(Vec3d v, double rad);
OBZ_API Vec3d obz_vec3d_rotate_y(Vec3d v, double rad);
OBZ_API Vec3d obz_vec3d_rotate_z(Vec3d v, double rad);

OBZ_API double obz_vec3d_len(Vec3d v);
OBZ_API Vec3d  obz_vec3d_norm(Vec3d v);
OBZ_API Vec3d  obz_vec3d_lerp(Vec3d a, Vec3d b, double t);

/*-------------------------------------------------------
    Vector conversions
-------------------------------------------------------*/

// explicit conversions from float to double and vice versa

OBZ_API Vec2d obz_vec2f_to_vec2d(Vec2f v);
OBZ_API Vec2f obz_vec2d_to_vec2f(Vec2d v);
OBZ_API Vec3d obz_vec3f_to_vec3d(Vec3f v);
OBZ_API Vec3f obz_vec3d_to_vec3f(Vec3d v);
OBZ_API Vec4d obz_vec4f_to_vec4d(Vec4f v);
OBZ_API Vec4f obz_vec4d_to_vec4f(Vec4d v);

// conversions between different dimensions
//
// Vec2 --> Vec3

OBZ_API Vec3f obz_vec2f_to_vec3f(Vec2f v, float z);
OBZ_API Vec3d obz_vec2d_to_vec3d(Vec2d v, double z);

// Vec2 --> Vec4

OBZ_API Vec4f obz_vec2f_to_vec4f(Vec2f v, float z, float w);
OBZ_API Vec4d obz_vec2d_to_vec4d(Vec2d v, double z, double w);

// Vec3 --> Vec2

OBZ_API Vec2f obz_vec3f_to_vec2f(Vec3f v);
OBZ_API Vec2d obz_vec3d_to_vec2d(Vec3d v);

// Vec3 --> Vec4

OBZ_API Vec4f obz_vec3f_to_vec4f(Vec3f v, float w);
OBZ_API Vec4d obz_vec3d_to_vec4d(Vec3d v, double w);

// Vec4 --> Vec2

OBZ_API Vec2f obz_vec4f_to_vec2f(Vec4f v);
OBZ_API Vec2d obz_vec4d_to_vec2d(Vec4d v);

// Vec4 --> Vec3

OBZ_API Vec3f obz_vec4f_to_vec3f(Vec4f v);
OBZ_API Vec3d obz_vec4d_to_vec3d(Vec4d v);

/*-------------------------------------------------------
    Type-Generic Macros
-------------------------------------------------------*/

/* Vec2 */
#define obz_vec2_add(a, b) _Generic((a), Vec2f: obz_vec2f_add, Vec2d: obz_vec2d_add)(a, b)
#define obz_vec2_sub(a, b) _Generic((a), Vec2f: obz_vec2f_sub, Vec2d: obz_vec2d_sub)(a, b)
#define obz_vec2_mul(a, b) _Generic((a), Vec2f: obz_vec2f_mul, Vec2d: obz_vec2d_mul)(a, b)
#define obz_vec2_div(a, b) _Generic((a), Vec2f: obz_vec2f_div, Vec2d: obz_vec2d_div)(a, b)

#define obz_vec2_addf(v, s) _Generic((v), Vec2f: obz_vec2f_addf, Vec2d: obz_vec2d_addf)(v, s)
#define obz_vec2_subf(v, s) _Generic((v), Vec2f: obz_vec2f_subf, Vec2d: obz_vec2d_subf)(v, s)
#define obz_vec2_mulf(v, s) _Generic((v), Vec2f: obz_vec2f_mulf, Vec2d: obz_vec2d_mulf)(v, s)
#define obz_vec2_divf(v, s) _Generic((v), Vec2f: obz_vec2f_divf, Vec2d: obz_vec2d_divf)(v, s)

#define obz_vec2_dot(a, b)     _Generic((a), Vec2f: obz_vec2f_dot, Vec2d: obz_vec2d_dot)(a, b)
#define obz_vec2_len(v)        _Generic((v), Vec2f: obz_vec2f_len, Vec2d: obz_vec2d_len)(v)
#define obz_vec2_norm(v)       _Generic((v), Vec2f: obz_vec2f_norm, Vec2d: obz_vec2d_norm)(v)
#define obz_vec2_lerp(a, b, t) _Generic((a), Vec2f: obz_vec2f_lerp, Vec2d: obz_vec2d_lerp)(a, b, t)
#define obz_vec2_rotate(v, rad) \
  _Generic((v), Vec2f: obz_vec2f_rotate, Vec2d: obz_vec2d_rotate)(v, rad)

/* Vec3 */
#define obz_vec3_add(a, b) _Generic((a), Vec3f: obz_vec3f_add, Vec3d: obz_vec3d_add)(a, b)
#define obz_vec3_sub(a, b) _Generic((a), Vec3f: obz_vec3f_sub, Vec3d: obz_vec3d_sub)(a, b)
#define obz_vec3_mul(a, b) _Generic((a), Vec3f: obz_vec3f_mul, Vec3d: obz_vec3d_mul)(a, b)
#define obz_vec3_div(a, b) _Generic((a), Vec3f: obz_vec3f_div, Vec3d: obz_vec3d_div)(a, b)

#define obz_vec3_addf(v, s) _Generic((v), Vec3f: obz_vec3f_addf, Vec2d: obz_vec3d_addf)(v, s)
#define obz_vec3_subf(v, s) _Generic((v), Vec3f: obz_vec3f_subf, Vec3d: obz_vec3d_subf)(v, s)
#define obz_vec3_mulf(v, s) _Generic((v), Vec3f: obz_vec3f_mulf, Vec3d: obz_vec3d_mulf)(v, s)
#define obz_vec3_divf(v, s) _Generic((v), Vec3f: obz_vec3f_divf, Vec3d: obz_vec3d_divf)(v, s)

#define obz_vec3_dot(a, b)     _Generic((a), Vec3f: obz_vec3f_dot, Vec3d: obz_vec3d_dot)(a, b)
#define obz_vec3_cross(a, b)   _Generic((a), Vec3f: obz_vec3f_cross, Vec3d: obz_vec3d_cross)(a, b)
#define obz_vec3_len(v)        _Generic((v), Vec3f: obz_vec3f_len, Vec3d: obz_vec3d_len)(v)
#define obz_vec3_norm(v)       _Generic((v), Vec3f: obz_vec3f_norm, Vec3d: obz_vec3d_norm)(v)
#define obz_vec3_lerp(a, b, t) _Generic((a), Vec3f: obz_vec3f_lerp, Vec3d: obz_vec3d_lerp)(a, b, t)
#define obz_vec3_rotate_x(v, rad) \
  _Generic((v), Vec3f: obz_vec3f_rotate_x, Vec3d: obz_vec3d_rotate_x)(v, rad)
#define obz_vec3_rotate_y(v, rad) \
  _Generic((v), Vec3f: obz_vec3f_rotate_y, Vec3d: obz_vec3d_rotate_y)(v, rad)
#define obz_vec3_rotate_z(v, rad) \
  _Generic((v), Vec3f: obz_vec3f_rotate_z, Vec3d: obz_vec3d_rotate_z)(v, rad)

/* Vector conversions */
#define obz_vec2_to_vec3(v, z) \
  _Generic((v), Vec2f: obz_vec2f_to_vec3f, Vec2d: obz_vec2d_to_vec3d)(v, z)

#define obz_vec2_to_vec4(v, z, w) \
  _Generic((v), Vec2f: obz_vec2f_to_vec4f, Vec2d: obz_vec2d_to_vec4d)(v, z, w)

#define obz_vec3_to_vec2(v) _Generic((v), Vec3f: obz_vec3f_to_vec2f, Vec3d: obz_vec3d_to_vec2d)(v)

#define obz_vec3_to_vec4(v, w) \
  _Generic((v), Vec3f: obz_vec3f_to_vec4f, Vec3d: obz_vec3d_to_vec4d)(v, w)

#define obz_vec4_to_vec2(v) _Generic((v), Vec4f: obz_vec4f_to_vec2f, Vec4d: obz_vec4d_to_vec2d)(v)

#define obz_vec4_to_vec3(v) _Generic((v), Vec4f: obz_vec4f_to_vec3f, Vec4d: obz_vec4d_to_vec3d)(v)

OBZ_END_CPP_DECLS
#endif
