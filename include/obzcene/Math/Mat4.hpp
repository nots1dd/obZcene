#pragma once

/**
 * @file Mat4.hpp
 * @brief 4x4 homogeneous matrix: view/projection transforms of obzcene::Camera.
 *
 * Row-major storage (``data[r*4+c]``) acting on #math::Vec4. The camera
 * combines them as `projection_matrix() * view_matrix()` (see
 * obzcene::Camera::view_projection) and the rasterizer applies that product to
 * every vertex. Perspective/orthographic projection, look-at, translation,
 * rotation and scaling factories are all provided. Aliases: #math::Mat4f,
 * #math::Mat4d.
 *
 * @see Mat3 (3x3 sub-block via #Mat4::upper3), Quaternion (to_mat4),
 *      Camera (primary consumer)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Mat3.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Math/Vec4.hpp"

#include <cassert>
#include <cmath>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A generic 4x4 homogeneous matrix.
 * @tparam T Scalar type (``float``/``double``).
 *
 * Stores 16 scalars in row-major order; `operator()(r,c)` indexes by
 * (row, column). Multiplication follows the convention `M * v`, so the right
 * operand is applied first. The default constructor builds the identity.
 */
template <typename T> struct Mat4
{
  T data[16]{};

  /** @brief Identity matrix by default. */
  constexpr Mat4()
      : data{T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0),
             T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1)}
  {
  }

  /** @brief Explicit constructor: scalar `diag` on the diagonal, zero elsewhere. */
  constexpr explicit Mat4(T diag)
      : data{diag, T(0), T(0), T(0), T(0), diag, T(0), T(0),
             T(0), T(0), diag, T(0), T(0), T(0), T(0), diag}
  {
  }

  /** @brief Component-wise constructor (row-major). */
  constexpr Mat4(T m00, T m01, T m02, T m03, T m10, T m11, T m12, T m13, T m20, T m21, T m22, T m23,
                 T m30, T m31, T m32, T m33)
      : data{m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33}
  {
  }

  /** @brief Explicit constructor from a raw 16-element row-major array. */
  explicit constexpr Mat4(const T (&arr)[16])
  {
    for (int i = 0; i < 16; ++i)
      data[i] = arr[i];
  }

  /** @brief Access element at row `r`, column `c`. */
  constexpr auto operator()(int r, int c) -> T& { return data[r * 4 + c]; }

  /** @brief Read element at row `r`, column `c`. */
  constexpr auto operator()(int r, int c) const -> T { return data[r * 4 + c]; }

  /** @brief Extract row `r` as a Vec4. */
  [[nodiscard]] constexpr auto row(int r) const -> Vec4<T>
  {
    return Vec4<T>(data[r * 4 + 0], data[r * 4 + 1], data[r * 4 + 2], data[r * 4 + 3]);
  }

  /** @brief Extract column `c` as a Vec4. */
  [[nodiscard]] constexpr auto col(int c) const -> Vec4<T>
  {
    return Vec4<T>(data[0 * 4 + c], data[1 * 4 + c], data[2 * 4 + c], data[3 * 4 + c]);
  }

  constexpr auto operator*(const Mat4& rhs) const -> Mat4
  {
    Mat4 result;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        result.data[i * 4 + j] =
          data[i * 4 + 0] * rhs.data[0 * 4 + j] + data[i * 4 + 1] * rhs.data[1 * 4 + j] +
          data[i * 4 + 2] * rhs.data[2 * 4 + j] + data[i * 4 + 3] * rhs.data[3 * 4 + j];
    return result;
  }

  constexpr auto operator*(const Vec4<T>& v) const -> Vec4<T>
  {
    return Vec4<T>(data[0] * v.x + data[1] * v.y + data[2] * v.z + data[3] * v.w,
                   data[4] * v.x + data[5] * v.y + data[6] * v.z + data[7] * v.w,
                   data[8] * v.x + data[9] * v.y + data[10] * v.z + data[11] * v.w,
                   data[12] * v.x + data[13] * v.y + data[14] * v.z + data[15] * v.w);
  }

  constexpr auto operator*=(const Mat4& rhs) -> Mat4&
  {
    *this = *this * rhs;
    return *this;
  }

  constexpr auto operator+(const Mat4& rhs) const -> Mat4
  {
    Mat4 result;
    for (int i = 0; i < 16; ++i)
      result.data[i] = data[i] + rhs.data[i];
    return result;
  }

  constexpr auto operator-(const Mat4& rhs) const -> Mat4
  {
    Mat4 result;
    for (int i = 0; i < 16; ++i)
      result.data[i] = data[i] - rhs.data[i];
    return result;
  }

  constexpr auto operator*(T s) const -> Mat4
  {
    Mat4 result;
    for (int i = 0; i < 16; ++i)
      result.data[i] = data[i] * s;
    return result;
  }

  constexpr auto operator/(T s) const -> Mat4
  {
    Mat4 result;
    for (int i = 0; i < 16; ++i)
      result.data[i] = data[i] / s;
    return result;
  }

  constexpr auto operator==(const Mat4& rhs) const -> bool
  {
    for (int i = 0; i < 16; ++i)
      if (data[i] != rhs.data[i])
        return false;
    return true;
  }

  constexpr auto operator!=(const Mat4& rhs) const -> bool { return !(*this == rhs); }

  /** @brief Transposed matrix (rows become columns). */
  [[nodiscard]] constexpr auto transpose() const -> Mat4
  {
    return Mat4(data[0], data[4], data[8], data[12], data[1], data[5], data[9], data[13], data[2],
                data[6], data[10], data[14], data[3], data[7], data[11], data[15]);
  }

  /**
   * @brief Determinant using 2x2 sub-determinant (Laplace expansion).
   * @tparam T Must be floating-point.
   */
  [[nodiscard]] constexpr auto determinant() const -> T
  {
    static_assert(std::is_floating_point_v<T>, "determinant requires a floating-point type");

    const T s0 = data[0] * data[5] - data[4] * data[1];
    const T s1 = data[0] * data[6] - data[4] * data[2];
    const T s2 = data[0] * data[7] - data[4] * data[3];
    const T s3 = data[1] * data[6] - data[5] * data[2];
    const T s4 = data[1] * data[7] - data[5] * data[3];
    const T s5 = data[2] * data[7] - data[6] * data[3];

    const T c5 = data[10] * data[15] - data[14] * data[11];
    const T c4 = data[9] * data[15] - data[13] * data[11];
    const T c3 = data[9] * data[14] - data[13] * data[10];
    const T c2 = data[8] * data[15] - data[12] * data[11];
    const T c1 = data[8] * data[14] - data[12] * data[10];
    const T c0 = data[8] * data[13] - data[12] * data[9];

    return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
  }

  /**
   * @brief Inverse using the adjugate block form.
   * @pre Determinant must be non-zero (asserted in debug builds).
   * @note Used by obzcene::Camera::inv_view_projection for unprojection.
   */
  [[nodiscard]] constexpr auto inverse() const -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "inverse requires a floating-point type");

    const T m00 = data[0], m01 = data[1], m02 = data[2], m03 = data[3];
    const T m10 = data[4], m11 = data[5], m12 = data[6], m13 = data[7];
    const T m20 = data[8], m21 = data[9], m22 = data[10], m23 = data[11];
    const T m30 = data[12], m31 = data[13], m32 = data[14], m33 = data[15];

    const T s0 = m00 * m11 - m10 * m01;
    const T s1 = m00 * m12 - m10 * m02;
    const T s2 = m00 * m13 - m10 * m03;
    const T s3 = m01 * m12 - m11 * m02;
    const T s4 = m01 * m13 - m11 * m03;
    const T s5 = m02 * m13 - m12 * m03;

    const T c5 = m22 * m33 - m32 * m23;
    const T c4 = m21 * m33 - m31 * m23;
    const T c3 = m21 * m32 - m31 * m22;
    const T c2 = m20 * m33 - m30 * m23;
    const T c1 = m20 * m32 - m30 * m22;
    const T c0 = m20 * m31 - m30 * m21;

    T det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    assert(det != T(0));
    T inv_det = T(1) / det;

    Mat4 result;
    result.data[0]  = (m11 * c5 - m12 * c4 + m13 * c3) * inv_det;
    result.data[1]  = (-m01 * c5 + m02 * c4 - m03 * c3) * inv_det;
    result.data[2]  = (m31 * s5 - m32 * s4 + m33 * s3) * inv_det;
    result.data[3]  = (-m21 * s5 + m22 * s4 - m23 * s3) * inv_det;
    result.data[4]  = (-m10 * c5 + m12 * c2 - m13 * c1) * inv_det;
    result.data[5]  = (m00 * c5 - m02 * c2 + m03 * c1) * inv_det;
    result.data[6]  = (-m30 * s5 + m32 * s2 - m33 * s1) * inv_det;
    result.data[7]  = (m20 * s5 - m22 * s2 + m23 * s1) * inv_det;
    result.data[8]  = (m10 * c4 - m11 * c2 + m13 * c0) * inv_det;
    result.data[9]  = (-m00 * c4 + m01 * c2 - m03 * c0) * inv_det;
    result.data[10] = (m30 * s4 - m31 * s2 + m33 * s0) * inv_det;
    result.data[11] = (-m20 * s4 + m21 * s2 - m23 * s0) * inv_det;
    result.data[12] = (-m10 * c3 + m11 * c1 - m12 * c0) * inv_det;
    result.data[13] = (m00 * c3 - m01 * c1 + m02 * c0) * inv_det;
    result.data[14] = (-m30 * s3 + m31 * s1 - m32 * s0) * inv_det;
    result.data[15] = (m20 * s3 - m21 * s1 + m22 * s0) * inv_det;

    return result;
  }

  /** @brief The upper-left 3x3 block, ignoring the translation/projection row. */
  [[nodiscard]] constexpr auto upper3() const -> Mat3<T>
  {
    return Mat3<T>(data[0], data[1], data[2], data[4], data[5], data[6], data[8], data[9],
                   data[10]);
  }

  /** @brief Identity matrix. */
  static constexpr auto identity() -> Mat4 { return Mat4(); }

  /** @brief Zero matrix. */
  static constexpr auto zero() -> Mat4 { return Mat4(T(0)); }

  /**
   * @brief Translation by (x,y,z); world movement for obzcene::Camera.
   * @note A translation is 1 on the diagonal and x/y/z in the last column.
   */
  static constexpr auto translation(T x, T y, T z) -> Mat4
  {
    return Mat4(T(1), T(0), T(0), x, T(0), T(1), T(0), y, T(0), T(0), T(1), z, T(0), T(0), T(0),
                T(1));
  }

  /** @brief Rotation around the X axis by `angle` radians. */
  static constexpr auto rotation_x(T angle) -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "rotation_x requires a floating-point type");
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Mat4(T(1), T(0), T(0), T(0), T(0), c, -s, T(0), T(0), s, c, T(0), T(0), T(0), T(0),
                T(1));
  }

  /** @brief Rotation around the Y axis by `angle` radians. */
  static constexpr auto rotation_y(T angle) -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "rotation_y requires a floating-point type");
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Mat4(c, T(0), s, T(0), T(0), T(1), T(0), T(0), -s, T(0), c, T(0), T(0), T(0), T(0),
                T(1));
  }

  /** @brief Rotation around the Z axis by `angle` radians. */
  static constexpr auto rotation_z(T angle) -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "rotation_z requires a floating-point type");
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Mat4(c, -s, T(0), T(0), s, c, T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0),
                T(1));
  }

  /**
   * @brief Rotation around an arbitrary (not necessarily unit) axis.
   * @param axis  Axis; it is normalized internally so it may be any length.
   * @param angle Rotation angle in radians (homogeneous form of Mat3::rotation_axis).
   */
  static constexpr auto rotation_axis(const Vec3<T>& axis, T angle) -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "rotation_axis requires a floating-point type");
    const Vec3<T> a   = axis.normalized();
    const T       c   = std::cos(angle);
    const T       s   = std::sin(angle);
    const T       t   = T(1) - c;
    const T       tx  = t * a.x;
    const T       ty  = t * a.y;
    const T       tz  = t * a.z;
    const T       txy = tx * a.y;
    const T       txz = tx * a.z;
    const T       tyz = ty * a.z;
    const T       sx  = s * a.x;
    const T       sy  = s * a.y;
    const T       sz  = s * a.z;
    return Mat4(tx * a.x + c, txy - sz, txz + sy, T(0), txy + sz, ty * a.y + c, tyz - sx, T(0),
                txz - sy, tyz + sx, tz * a.z + c, T(0), T(0), T(0), T(0), T(1));
  }

  /** @brief Non-uniform scale matrix (per-axis factors, homogeneous last row). */
  static constexpr auto scaling(T sx, T sy, T sz) -> Mat4
  {
    return Mat4(sx, T(0), T(0), T(0), T(0), sy, T(0), T(0), T(0), T(0), sz, T(0), T(0), T(0), T(0),
                T(1));
  }

  /**
   * @brief Perspective projection matrix (right-handed, depth in [-1,1]).
   * @param fov_y   Vertical field of view in radians.
   * @param aspect  Width / height ratio.
   * @param near_val,far_val  Near and far clip distances (both > 0).
   * @note The `w` it emits for each vertex is what the rasterizer uses for
   *       perspective-correct interpolation (see obzcene::rasterize and
   *       #math::raster_detail::perspective_blend).
   */
  static constexpr auto perspective(T fov_y, T aspect, T near_val, T far_val) -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "perspective requires a floating-point type");
    const T f  = T(1) / std::tan(fov_y / T(2));
    const T nf = near_val - far_val;
    return Mat4(f / aspect, T(0), T(0), T(0), T(0), f, T(0), T(0), T(0), T(0),
                (far_val + near_val) / nf, T(2) * far_val * near_val / nf, T(0), T(0), T(-1), T(0));
  }

  /**
   * @brief Orthographic projection matrix (depth in [-1,1]).
   * @param left,right,bottom,top  View volume in x/y.
   * @param near_val,far_val       Near and far clip distances.
   */
  static constexpr auto ortho(T left, T right, T bottom, T top, T near_val, T far_val) -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "ortho requires a floating-point type");
    return Mat4(T(2) / (right - left), T(0), T(0), -(right + left) / (right - left), T(0),
                T(2) / (top - bottom), T(0), -(top + bottom) / (top - bottom), T(0), T(0),
                T(-2) / (far_val - near_val), -(far_val + near_val) / (far_val - near_val), T(0),
                T(0), T(0), T(1));
  }

  /**
   * @brief View matrix looking from `eye` towards `center`, with `up` hint.
   * @note Convention matches the camera basis used by obzcene::Camera::forward
   *       (forward points -Z when looking down -Z), and
   *       obzcene::Camera::view_matrix calls this directly.
   */
  static constexpr auto look_at(const Vec3<T>& eye, const Vec3<T>& center, const Vec3<T>& up)
    -> Mat4
  {
    static_assert(std::is_floating_point_v<T>, "look_at requires a floating-point type");
    const Vec3<T> f = (center - eye).normalized();
    const Vec3<T> s = cross(f, up).normalized();
    const Vec3<T> u = cross(s, f);
    return Mat4(s.x, s.y, s.z, -dot(s, eye), u.x, u.y, u.z, -dot(u, eye), -f.x, -f.y, -f.z,
                dot(f, eye), T(0), T(0), T(0), T(1));
  }

  /**
   * @brief Viewport transform mapping NDC ([-1,1]^2) into pixel/`z` space.
   * @note Y is flipped (NDC y grows up, screen y grows down), which mirrors the
   *       screen mapping performed by obzcene::rasterize.
   */
  static constexpr auto viewport(T x, T y, T width, T height, T min_depth, T max_depth) -> Mat4
  {
    return Mat4(width / T(2), T(0), T(0), x + width / T(2), T(0), -height / T(2), T(0),
                y + height / T(2), T(0), T(0), (max_depth - min_depth) / T(2),
                (max_depth + min_depth) / T(2), T(0), T(0), T(0), T(1));
  }

  /** @brief Approximate element-wise equality within `eps` (see #math::is_almost_equal). */
  [[nodiscard]] constexpr auto is_almost_equal(const Mat4& rhs, T eps = default_epsilon<T>()) const
    -> bool
  {
    for (int i = 0; i < 16; ++i)
      if (!math::is_almost_equal(data[i], rhs.data[i], eps))
        return false;
    return true;
  }
};

/** @brief 4x4 matrix alias using `float`. */
using Mat4f = Mat4<float>;
/** @brief 4x4 matrix alias using `double`. */
using Mat4d = Mat4<double>;

} // namespace obzcene::math
