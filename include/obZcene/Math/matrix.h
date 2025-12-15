#ifndef OBZ_MATH_MATRIX_H
#define OBZ_MATH_MATRIX_H

#include "Math/vec.h"
#include "obz_macros.h"

OBZ_BEGIN_CPP_DECLS

typedef struct
{
  float m[4][4];
} Mat4f;

typedef struct
{
  double m[4][4];
} Mat4d;

OBZ_API Mat4f obz_mat4f_identity();
OBZ_API Mat4d obz_mat4d_identity();

OBZ_API Vec4f obz_mat4f_mul_vec4f(Mat4f m, Vec4f v);
OBZ_API Mat4f obz_mat4f_mul_mat4f(Mat4f a, Mat4f b);

OBZ_API Mat4f obz_mat4f_make_scale(float sx, float sy, float sz);
OBZ_API Mat4f obz_mat4f_make_translation(float tx, float ty, float tz);

OBZ_API Vec4f obz_mat4f_mul_vec4f_project(Mat4f mat_proj, Vec4f v);

OBZ_API Mat4f obz_mat4f_make_ortho(float l, float b, float n, float r, float t, float f);
OBZ_API Mat4f obz_mat4f_make_perspective(float n, float f);
OBZ_API Mat4f obz_mat4f_make_projection(float fov, float aspect_ratio, float near, float far);

OBZ_API Mat4f obz_mat4f_look_at(Vec3f eye, Vec3f target, Vec3f up);

OBZ_API Mat4f obz_mat4f_make_rotation_x(float angle);
OBZ_API Mat4f obz_mat4f_make_rotation_y(float angle);
OBZ_API Mat4f obz_mat4f_make_rotation_z(float angle);

OBZ_END_CPP_DECLS

#endif
