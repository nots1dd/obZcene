#include "Math/vec.h"
#include "Math/simd/x86/sqrt.h"
#include <math.h>

/*-------------------------------------------------------
    Vec2f Implementation
-------------------------------------------------------*/
Vec2f obz_vec2f_add(Vec2f a, Vec2f b) { return (Vec2f){a.x + b.x, a.y + b.y}; }
Vec2f obz_vec2f_sub(Vec2f a, Vec2f b) { return (Vec2f){a.x - b.x, a.y - b.y}; }
Vec2f obz_vec2f_mul(Vec2f a, Vec2f b) { return (Vec2f){a.x * b.x, a.y * b.y}; }
Vec2f obz_vec2f_div(Vec2f a, Vec2f b) { return (Vec2f){a.x / b.x, a.y / b.y}; }

Vec2f obz_vec2f_addf(Vec2f v, float s) { return (Vec2f){v.x + s, v.y + s}; }
Vec2f obz_vec2f_subf(Vec2f v, float s) { return (Vec2f){v.x - s, v.y - s}; }
Vec2f obz_vec2f_mulf(Vec2f v, float s) { return (Vec2f){v.x * s, v.y * s}; }
Vec2f obz_vec2f_divf(Vec2f v, float s) { return (Vec2f){v.x / s, v.y / s}; }

Vec2f obz_vec2f_scale(Vec2f v, float s) { return (Vec2f){v.x * s, v.y * s}; }
float obz_vec2f_dot(Vec2f a, Vec2f b) { return a.x * b.x + a.y * b.y; }

float obz_vec2f_len(Vec2f v) { return obz_sqrtf(v.x * v.x + v.y * v.y); }

Vec2f obz_vec2f_norm(Vec2f v)
{
  float l = obz_vec2f_len(v);
  return l > 0 ? (Vec2f){v.x / l, v.y / l} : v;
}

Vec2f obz_vec2f_lerp(Vec2f a, Vec2f b, float t)
{
  return (Vec2f){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

Vec2f obz_vec2f_rotate(Vec2f v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec2f){v.x * c - v.y * s, v.x * s + v.y * c};
}

/*-------------------------------------------------------
    Vec2d Implementation
-------------------------------------------------------*/
Vec2d obz_vec2d_add(Vec2d a, Vec2d b) { return (Vec2d){a.x + b.x, a.y + b.y}; }
Vec2d obz_vec2d_sub(Vec2d a, Vec2d b) { return (Vec2d){a.x - b.x, a.y - b.y}; }
Vec2d obz_vec2d_mul(Vec2d a, Vec2d b) { return (Vec2d){a.x * b.x, a.y * b.y}; }
Vec2d obz_vec2d_div(Vec2d a, Vec2d b) { return (Vec2d){a.x / b.x, a.y / b.y}; }

Vec2d obz_vec2d_addf(Vec2d v, double s) { return (Vec2d){v.x + s, v.y + s}; }
Vec2d obz_vec2d_subf(Vec2d v, double s) { return (Vec2d){v.x - s, v.y - s}; }
Vec2d obz_vec2d_mulf(Vec2d v, double s) { return (Vec2d){v.x * s, v.y * s}; }
Vec2d obz_vec2d_divf(Vec2d v, double s) { return (Vec2d){v.x / s, v.y / s}; }

Vec2d  obz_vec2d_scale(Vec2d v, double s) { return (Vec2d){v.x * s, v.y * s}; }
double obz_vec2d_dot(Vec2d a, Vec2d b) { return a.x * b.x + a.y * b.y; }

double obz_vec2d_len(Vec2d v)
{
  double s = v.x * v.x + v.y * v.y;
  return obz_sqrtf(s);
}

Vec2d obz_vec2d_norm(Vec2d v)
{
  double l = obz_vec2d_len(v);
  return l > 0 ? (Vec2d){v.x / l, v.y / l} : v;
}

Vec2d obz_vec2d_lerp(Vec2d a, Vec2d b, double t)
{
  return (Vec2d){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

Vec2d obz_vec2d_rotate(Vec2d v, double rad)
{
  double c = cos(rad), s = sin(rad);
  return (Vec2d){v.x * c - v.y * s, v.x * s + v.y * c};
}

/*-------------------------------------------------------
    Vec3f Implementation
-------------------------------------------------------*/
Vec3f obz_vec3f_add(Vec3f a, Vec3f b) { return (Vec3f){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3f obz_vec3f_sub(Vec3f a, Vec3f b) { return (Vec3f){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3f obz_vec3f_mul(Vec3f a, Vec3f b) { return (Vec3f){a.x * b.x, a.y * b.y, a.z * b.z}; }
Vec3f obz_vec3f_div(Vec3f a, Vec3f b) { return (Vec3f){a.x / b.x, a.y / b.y, a.z / b.z}; }

Vec3f obz_vec3f_addf(Vec3f v, float s) { return (Vec3f){v.x + s, v.y + s, v.z + s}; }
Vec3f obz_vec3f_subf(Vec3f v, float s) { return (Vec3f){v.x - s, v.y - s, v.z - s}; }
Vec3f obz_vec3f_mulf(Vec3f v, float s) { return (Vec3f){v.x * s, v.y * s, v.z * s}; }
Vec3f obz_vec3f_divf(Vec3f v, float s) { return (Vec3f){v.x / s, v.y / s, v.z / s}; }

Vec3f obz_vec3f_scale(Vec3f v, float s) { return (Vec3f){v.x * s, v.y * s, v.z * s}; }
float obz_vec3f_dot(Vec3f a, Vec3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3f obz_vec3f_cross(Vec3f a, Vec3f b)
{
  return (Vec3f){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3f obz_vec3f_rotate_x(Vec3f v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec3f){v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

Vec3f obz_vec3f_rotate_y(Vec3f v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec3f){v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

Vec3f obz_vec3f_rotate_z(Vec3f v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec3f){v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

float obz_vec3f_len(Vec3f v) { return obz_sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

Vec3f obz_vec3f_norm(Vec3f v)
{
  float l = obz_vec3f_len(v);
  return l > 0 ? (Vec3f){v.x / l, v.y / l, v.z / l} : v;
}

Vec3f obz_vec3f_lerp(Vec3f a, Vec3f b, float t)
{
  return (Vec3f){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

/*-------------------------------------------------------
    Vec3d Implementation
-------------------------------------------------------*/
Vec3d obz_vec3d_add(Vec3d a, Vec3d b) { return (Vec3d){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3d obz_vec3d_sub(Vec3d a, Vec3d b) { return (Vec3d){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3d obz_vec3d_mul(Vec3d a, Vec3d b) { return (Vec3d){a.x * b.x, a.y * b.y, a.z * b.z}; }
Vec3d obz_vec3d_div(Vec3d a, Vec3d b) { return (Vec3d){a.x / b.x, a.y / b.y, a.z / b.z}; }

Vec3d obz_vec3d_addf(Vec3d v, double s) { return (Vec3d){v.x + s, v.y + s, v.z + s}; }
Vec3d obz_vec3d_subf(Vec3d v, double s) { return (Vec3d){v.x - s, v.y - s, v.z - s}; }
Vec3d obz_vec3d_mulf(Vec3d v, double s) { return (Vec3d){v.x * s, v.y * s, v.z * s}; }
Vec3d obz_vec3d_divf(Vec3d v, double s) { return (Vec3d){v.x / s, v.y / s, v.z / s}; }

Vec3d  obz_vec3d_scale(Vec3d v, double s) { return (Vec3d){v.x * s, v.y * s, v.z * s}; }
double obz_vec3d_dot(Vec3d a, Vec3d b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3d obz_vec3d_cross(Vec3d a, Vec3d b)
{
  return (Vec3d){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3d obz_vec3d_rotate_x(Vec3d v, double rad)
{
  double c = cos(rad), s = sin(rad);
  return (Vec3d){v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

Vec3d obz_vec3d_rotate_y(Vec3d v, double rad)
{
  double c = cos(rad), s = sin(rad);
  return (Vec3d){v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

Vec3d obz_vec3d_rotate_z(Vec3d v, double rad)
{
  double c = cos(rad), s = sin(rad);
  return (Vec3d){v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

double obz_vec3d_len(Vec3d v) { return obz_sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

Vec3d obz_vec3d_norm(Vec3d v)
{
  double l = obz_vec3d_len(v);
  return l > 0 ? (Vec3d){v.x / l, v.y / l, v.z / l} : v;
}

Vec3d obz_vec3d_lerp(Vec3d a, Vec3d b, double t)
{
  return (Vec3d){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

/*-------------------------------------------------------
    Vector conversions
-------------------------------------------------------*/

Vec2f obz_vec2d_to_vec2f(Vec2d v) { return (Vec2f){(float)v.x, (float)v.y}; }
Vec2d obz_vec2f_to_vec2d(Vec2f v) { return (Vec2d){(double)v.x, (double)v.y}; }

Vec3f obz_vec3d_to_vec3f(Vec3d v) { return (Vec3f){(float)v.x, (float)v.y, (float)v.z}; }
Vec3d obz_vec3f_to_vec3d(Vec3f v) { return (Vec3d){(double)v.x, (double)v.y, (double)v.z}; }

Vec4f obz_vec4d_to_vec4f(Vec4d v)
{
  return (Vec4f){(float)v.x, (float)v.y, (float)v.z, (float)v.w};
}
Vec4d obz_vec4f_to_vec4d(Vec4f v)
{
  return (Vec4d){(double)v.x, (double)v.y, (double)v.z, (double)v.w};
}

// conversions between different dimensions
//
// Vec2 --> Vec3

Vec3f obz_vec2f_to_vec3f(Vec2f v, float z) { return (Vec3f){v.x, v.y, z}; }
Vec3d obz_vec2d_to_vec3d(Vec2d v, double z) { return (Vec3d){v.x, v.y, z}; }

// Vec2 --> Vec4

Vec4f obz_vec2f_to_vec4f(Vec2f v, float z, float w) { return (Vec4f){v.x, v.y, z, w}; }
Vec4d obz_vec2d_to_vec4d(Vec2d v, double z, double w) { return (Vec4d){v.x, v.y, z, w}; }

// Vec3 --> Vec2

Vec2d obz_vec3d_to_vec2d(Vec3d v) { return (Vec2d){v.x, v.y}; }
Vec2f obz_vec3f_to_vec2f(Vec3f v) { return (Vec2f){v.x, v.y}; }

// Vec3 --> Vec4

Vec4f obz_vec3f_to_vec4f(Vec3f v, float w) { return (Vec4f){v.x, v.y, v.z, w}; }
Vec4d obz_vec3d_to_vec4d(Vec3d v, double w) { return (Vec4d){v.x, v.y, v.z, w}; }

// Vec4 --> Vec2

Vec2f obz_vec4f_to_vec2f(Vec4f v) { return (Vec2f){v.x, v.y}; }
Vec2d obz_vec4d_to_vec2d(Vec4d v) { return (Vec2d){v.x, v.y}; }

// Vec4 --> Vec3

Vec3f obz_vec4f_to_vec3f(Vec4f v) { return (Vec3f){v.x, v.y, v.z}; }
Vec3d obz_vec4d_to_vec3d(Vec4d v) { return (Vec3d){v.x, v.y, v.z}; }
