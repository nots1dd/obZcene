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

// double

OBZ_API Mat4d obz_mat4d_identity();

OBZ_API Vec4d obz_mat4d_mul_vec4d(Mat4d m, Vec4d v);
OBZ_API Mat4d obz_mat4d_mul_mat4d(Mat4d a, Mat4d b);

OBZ_API Mat4d obz_mat4d_make_scale(double sx, double sy, double sz);
OBZ_API Mat4d obz_mat4d_make_translation(double tx, double ty, double tz);

OBZ_API Vec4d obz_mat4d_mul_vec4d_project(Mat4d mat_proj, Vec4d v);

OBZ_API Mat4d obz_mat4d_make_ortho(double l, double b, double n, double r, double t, double f);
OBZ_API Mat4d obz_mat4d_make_perspective(double n, double f);
OBZ_API Mat4d obz_mat4d_make_projection(double fov, double aspect_ratio, double near, double far);

OBZ_API Mat4d obz_mat4d_look_at(Vec3d eye, Vec3d target, Vec3d up);

OBZ_API Mat4d obz_mat4d_make_rotation_x(double angle);
OBZ_API Mat4d obz_mat4d_make_rotation_y(double angle);
OBZ_API Mat4f obz_mat4d_make_rotation_z(double angle);

#define obz_mat4_identity(T) \
  _Generic(((T)0), float: obz_mat4f_identity, double: obz_mat4d_identity)()

#define obz_mat4_make_scale(T, sx, sy, sz)                                                      \
  _Generic(((T)0), float: obz_mat4f_make_scale, double: obz_mat4d_make_scale)((T)(sx), (T)(sy), \
                                                                              (T)(sz))
#define obz_mat4_mul_vec_project(T, mat, v) \
  _Generic(((T)0), float: obz_mat4f_mul_vec4f_project, double: obz_mat4d_mul_vec4d_project)(mat, v)

#define obz_mat4_make_ortho(T, l, b, n, r, t, f)                               \
  _Generic(((T)0), float: obz_mat4f_make_ortho, double: obz_mat4d_make_ortho)( \
    (T)(l), (T)(b), (T)(n), (T)(r), (T)(t), (T)(f))

#define obz_mat4_make_perspective(T, n, f)                                                        \
  _Generic(((T)0), float: obz_mat4f_make_perspective, double: obz_mat4d_make_perspective)((T)(n), \
                                                                                          (T)(f))

#define obz_mat4_make_projection(T, fov, aspect, n, f)                                   \
  _Generic(((T)0), float: obz_mat4f_make_projection, double: obz_mat4d_make_projection)( \
    (T)(fov), (T)(aspect), (T)(n), (T)(f))

#define obz_mat4_look_at(T, eye, target, up) \
  _Generic(((T)0), float: obz_mat4f_look_at, double: obz_mat4d_look_at)(eye, target, up)

#define obz_mat4_mul(a, b) \
  _Generic((a), Mat4f: obz_mat4f_mul_mat4f, Mat4d: obz_mat4d_mul_mat4d)(a, b)

#define obz_mat4_make_translation(T, tx, ty, tz)                                           \
  _Generic(((T)0), float: obz_mat4f_make_translation, double: obz_mat4d_make_translation)( \
    (T)(tx), (T)(ty), (T)(tz))

#define obz_mat4_make_rotation_x(T, a) \
  _Generic(((T)0), float: obz_mat4f_make_rotation_x, double: obz_mat4d_make_rotation_x)((T)(a))

#define obz_mat4_make_rotation_y(T, a) \
  _Generic(((T)0), float: obz_mat4f_make_rotation_y, double: obz_mat4d_make_rotation_y)((T)(a))

#define obz_mat4_make_rotation_z(T, a) \
  _Generic(((T)0), float: obz_mat4f_make_rotation_z, double: obz_mat4d_make_rotation_z)((T)(a))

OBZ_END_CPP_DECLS

#endif
