#ifndef OBZ_UTILS_VEC_H
#define OBZ_UTILS_VEC_H

#include "Math/simd/x86/sqrt.h"
#include "obz_macros.h"

OBZ_BEGIN_CPP_DECLS

typedef struct
{
  float x, y;
} Vec2;
typedef struct
{
  float x, y, z;
} Vec3;

inline static Vec2 obz_vec2(float x, float y) { return (Vec2){x, y}; }
inline static Vec3 obz_vec3(float x, float y, float z) { return (Vec3){x, y, z}; }

/* Vec2 ops */
OBZ_API Vec2 obz_vec2_add(Vec2 a, Vec2 b);
OBZ_API Vec2 obz_vec2_sub(Vec2 a, Vec2 b);
OBZ_API Vec2 obz_vec2_mul(Vec2 a, Vec2 b);
OBZ_API Vec2 obz_vec2_div(Vec2 a, Vec2 b);

OBZ_API Vec2 obz_vec2_addf(Vec2 v, float s);
OBZ_API Vec2 obz_vec2_subf(Vec2 v, float s);
OBZ_API Vec2 obz_vec2_mulf(Vec2 v, float s);
OBZ_API Vec2 obz_vec2_divf(Vec2 v, float s);

OBZ_API Vec2  obz_vec2_scale(Vec2 v, float s);
OBZ_API float obz_vec2_dot(Vec2 a, Vec2 b);
OBZ_API float obz_vec2_len(Vec2 v);
OBZ_API Vec2  obz_vec2_norm(Vec2 v);
OBZ_API Vec2  obz_vec2_lerp(Vec2 a, Vec2 b, float t);
OBZ_API Vec2  obz_vec2_rotate(Vec2 v, float rad);

/* Vec3 ops */
OBZ_API Vec3 obz_vec3_add(Vec3 a, Vec3 b);
OBZ_API Vec3 obz_vec3_sub(Vec3 a, Vec3 b);
OBZ_API Vec3 obz_vec3_mul(Vec3 a, Vec3 b);
OBZ_API Vec3 obz_vec3_div(Vec3 a, Vec3 b);

OBZ_API Vec3  obz_vec3_scale(Vec3 v, float s);
OBZ_API float obz_vec3_dot(Vec3 a, Vec3 b);
OBZ_API Vec3  obz_vec3_cross(Vec3 a, Vec3 b);

OBZ_API Vec3 obz_vec3_rotate_x(Vec3 v, float rad);
OBZ_API Vec3 obz_vec3_rotate_y(Vec3 v, float rad);
OBZ_API Vec3 obz_vec3_rotate_z(Vec3 v, float rad);

/* scalar */
OBZ_API Vec3 obz_vec3_addf(Vec3 v, float s);
OBZ_API Vec3 obz_vec3_subf(Vec3 v, float s);
OBZ_API Vec3 obz_vec3_mulf(Vec3 v, float s);
OBZ_API Vec3 obz_vec3_divf(Vec3 v, float s);

OBZ_API float obz_vec3_len(Vec3 v);
OBZ_API Vec3  obz_vec3_norm(Vec3 v);
OBZ_API Vec3  obz_vec3_lerp(Vec3 a, Vec3 b, float t);

OBZ_END_CPP_DECLS

#endif
