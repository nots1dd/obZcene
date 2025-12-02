#include "Utils/vec.h"
#include <math.h>

Vec2 obz_vec2_add(Vec2 a, Vec2 b) { return (Vec2){a.x + b.x, a.y + b.y}; }
Vec2 obz_vec2_sub(Vec2 a, Vec2 b) { return (Vec2){a.x - b.x, a.y - b.y}; }
Vec2 obz_vec2_mul(Vec2 a, Vec2 b) { return (Vec2){a.x * b.x, a.y * b.y}; }
Vec2 obz_vec2_div(Vec2 a, Vec2 b) { return (Vec2){a.x / b.x, a.y / b.y}; }

Vec2  obz_vec2_scale(Vec2 v, float s) { return (Vec2){v.x * s, v.y * s}; }
float obz_vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
float obz_vec2_len(Vec2 v) { return OBZ__SIMD_sqrt_scalar_sse(v.x * v.x + v.y * v.y); }

Vec2 obz_vec2_norm(Vec2 v)
{
  float l = obz_vec2_len(v);
  return l > 0 ? (Vec2){v.x / l, v.y / l} : v;
}

Vec2 obz_vec2_lerp(Vec2 a, Vec2 b, float t)
{
  return (Vec2){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

Vec2 obz_vec2_rotate(Vec2 v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec2){v.x * c - v.y * s, v.x * s + v.y * c};
}

Vec3 obz_vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 obz_vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 obz_vec3_mul(Vec3 a, Vec3 b) { return (Vec3){a.x * b.x, a.y * b.y, a.z * b.z}; }
Vec3 obz_vec3_div(Vec3 a, Vec3 b) { return (Vec3){a.x / b.x, a.y / b.y, a.z / b.z}; }

/* scalar */
Vec2 obz_vec2_addf(Vec2 v, float s) { return (Vec2){v.x + s, v.y + s}; }

Vec2 obz_vec2_subf(Vec2 v, float s) { return (Vec2){v.x - s, v.y - s}; }

Vec2 obz_vec2_mulf(Vec2 v, float s) { return (Vec2){v.x * s, v.y * s}; }

Vec2 obz_vec2_divf(Vec2 v, float s) { return (Vec2){v.x / s, v.y / s}; }

Vec3  obz_vec3_scale(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
float obz_vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 obz_vec3_cross(Vec3 a, Vec3 b)
{
  return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 obz_vec3_rotate_x(Vec3 v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec3){v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

Vec3 obz_vec3_rotate_y(Vec3 v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec3){v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

Vec3 obz_vec3_rotate_z(Vec3 v, float rad)
{
  float c = cosf(rad), s = sinf(rad);
  return (Vec3){v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

Vec3 obz_vec3_addf(Vec3 v, float s) { return (Vec3){v.x + s, v.y + s, v.z + s}; }

Vec3 obz_vec3_subf(Vec3 v, float s) { return (Vec3){v.x - s, v.y - s, v.z - s}; }

Vec3 obz_vec3_mulf(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }

Vec3 obz_vec3_divf(Vec3 v, float s) { return (Vec3){v.x / s, v.y / s, v.z / s}; }

float obz_vec3_len(Vec3 v) { return OBZ__SIMD_sqrt_scalar_sse(v.x * v.x + v.y * v.y + v.z * v.z); }

Vec3 obz_vec3_norm(Vec3 v)
{
  float l = obz_vec3_len(v);
  return l > 0 ? (Vec3){v.x / l, v.y / l, v.z / l} : v;
}

Vec3 obz_vec3_lerp(Vec3 a, Vec3 b, float t)
{
  return (Vec3){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
