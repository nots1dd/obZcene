#pragma once

/**
 * @file Mat3.hpp
 * @brief 3x3 matrix with transforms and compositions for rotation/scaling.
 *
 * Row-major storage (``data[r*3+c]``). Mainly used to represent rotations:
 * math::Quat<T>::to_mat3 converts a quaternion into this type, and the static
 * factory functions build rotation and scaling matrices directly. Aliases:
 * #math::Mat3f, #math::Mat3d.
 *
 * @see Mat4 (the full homogeneous transform used by obzcene::Camera),
 *      Quaternion (interpolation that produces rotations)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cassert>
#include <cmath>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A generic 3x3 matrix.
 * @tparam T Scalar type (``float``/``double``).
 *
 * Store is a row-major array of nine scalars; `operator()(r,c)` indexes by
 * (row, column). The default constructor builds the identity matrix.
 */
template <typename T> struct Mat3
{
  T data[9]{};

  /** @brief Identity matrix by default. */
  constexpr Mat3() : data{T(1), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(1)} {}

  /** @brief Explicit constructor: scalar `diag` on the diagonal, zero elsewhere. */
  constexpr explicit Mat3(T diag) : data{diag, T(0), T(0), T(0), diag, T(0), T(0), T(0), diag} {}

  /** @brief Component-wise constructor (row-major). */
  constexpr Mat3(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22)
      : data{m00, m01, m02, m10, m11, m12, m20, m21, m22}
  {
  }

  /** @brief Explicit constructor from a raw 9-element row-major array. */
  explicit constexpr Mat3(const T (&arr)[9])
  {
    for (int i = 0; i < 9; ++i)
      data[i] = arr[i];
  }

  /** @brief Access element at row `r`, column `c`. */
  constexpr auto operator()(int r, int c) -> T& { return data[r * 3 + c]; }

  /** @brief Read element at row `r`, column `c`. */
  constexpr auto operator()(int r, int c) const -> T { return data[r * 3 + c]; }

  /** @brief Extract row `r` as a Vec3. */
  [[nodiscard]] constexpr auto row(int r) const -> Vec3<T>
  {
    return Vec3<T>(data[r * 3 + 0], data[r * 3 + 1], data[r * 3 + 2]);
  }

  /** @brief Extract column `c` as a Vec3. */
  [[nodiscard]] constexpr auto col(int c) const -> Vec3<T>
  {
    return Vec3<T>(data[0 * 3 + c], data[1 * 3 + c], data[2 * 3 + c]);
  }

  constexpr auto operator*(const Mat3& rhs) const -> Mat3
  {
    Mat3 result;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        result.data[i * 3 + j] = data[i * 3 + 0] * rhs.data[0 * 3 + j] +
                                 data[i * 3 + 1] * rhs.data[1 * 3 + j] +
                                 data[i * 3 + 2] * rhs.data[2 * 3 + j];
    return result;
  }

  constexpr auto operator*(const Vec3<T>& v) const -> Vec3<T>
  {
    return Vec3<T>(data[0] * v.x + data[1] * v.y + data[2] * v.z,
                   data[3] * v.x + data[4] * v.y + data[5] * v.z,
                   data[6] * v.x + data[7] * v.y + data[8] * v.z);
  }

  constexpr auto operator*=(const Mat3& rhs) -> Mat3&
  {
    *this = *this * rhs;
    return *this;
  }

  constexpr auto operator+(const Mat3& rhs) const -> Mat3
  {
    Mat3 result;
    for (int i = 0; i < 9; ++i)
      result.data[i] = data[i] + rhs.data[i];
    return result;
  }

  constexpr auto operator-(const Mat3& rhs) const -> Mat3
  {
    Mat3 result;
    for (int i = 0; i < 9; ++i)
      result.data[i] = data[i] - rhs.data[i];
    return result;
  }

  constexpr auto operator*(T s) const -> Mat3
  {
    Mat3 result;
    for (int i = 0; i < 9; ++i)
      result.data[i] = data[i] * s;
    return result;
  }

  constexpr auto operator/(T s) const -> Mat3
  {
    Mat3 result;
    for (int i = 0; i < 9; ++i)
      result.data[i] = data[i] / s;
    return result;
  }

  constexpr auto operator==(const Mat3& rhs) const -> bool
  {
    for (int i = 0; i < 9; ++i)
      if (data[i] != rhs.data[i])
        return false;
    return true;
  }

  constexpr auto operator!=(const Mat3& rhs) const -> bool { return !(*this == rhs); }

  /** @brief Transposed matrix (rows become columns). */
  [[nodiscard]] constexpr auto transpose() const -> Mat3
  {
    return Mat3(data[0], data[3], data[6], data[1], data[4], data[7], data[2], data[5], data[8]);
  }

  /**
   * @brief Determinant via the Laplace expansion.
   * @tparam T Must be floating-point.
   */
  [[nodiscard]] constexpr auto determinant() const -> T
  {
    static_assert(std::is_floating_point_v<T>, "determinant requires a floating-point type");
    return data[0] * (data[4] * data[8] - data[5] * data[7]) -
           data[1] * (data[3] * data[8] - data[5] * data[6]) +
           data[2] * (data[3] * data[7] - data[4] * data[6]);
  }

  /**
   * @brief Inverse via the adjugate divided by the determinant.
   * @pre Determinant must be non-zero (asserted in debug builds).
   * @note Useful to invert pure rotations like `view_matrix()` upper 3x3.
   */
  [[nodiscard]] constexpr auto inverse() const -> Mat3
  {
    static_assert(std::is_floating_point_v<T>, "inverse requires a floating-point type");
    const T det = determinant();
    assert(det != T(0));
    const T inv_det = T(1) / det;
    return Mat3((data[4] * data[8] - data[5] * data[7]) * inv_det,
                (data[2] * data[7] - data[1] * data[8]) * inv_det,
                (data[1] * data[5] - data[2] * data[4]) * inv_det,
                (data[5] * data[6] - data[3] * data[8]) * inv_det,
                (data[0] * data[8] - data[2] * data[6]) * inv_det,
                (data[2] * data[3] - data[0] * data[5]) * inv_det,
                (data[3] * data[7] - data[4] * data[6]) * inv_det,
                (data[1] * data[6] - data[0] * data[7]) * inv_det,
                (data[0] * data[4] - data[1] * data[3]) * inv_det);
  }

  static constexpr auto identity() -> Mat3 { return Mat3(); }

  /** @brief Zero matrix. */
  static constexpr auto zero() -> Mat3 { return Mat3(T(0)); }

  /**
   * @brief Rotation around the X axis by `angle` radians.
   * @tparam T Must be floating-point.
   */
  static constexpr auto rotation_x(T angle) -> Mat3
  {
    static_assert(std::is_floating_point_v<T>, "rotation_x requires a floating-point type");
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Mat3(T(1), T(0), T(0), T(0), c, -s, T(0), s, c);
  }

  /** @brief Rotation around the Y axis by `angle` radians. */
  static constexpr auto rotation_y(T angle) -> Mat3
  {
    static_assert(std::is_floating_point_v<T>, "rotation_y requires a floating-point type");
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Mat3(c, T(0), s, T(0), T(1), T(0), -s, T(0), c);
  }

  /** @brief Rotation around the Z axis by `angle` radians. */
  static constexpr auto rotation_z(T angle) -> Mat3
  {
    static_assert(std::is_floating_point_v<T>, "rotation_z requires a floating-point type");
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Mat3(c, -s, T(0), s, c, T(0), T(0), T(0), T(1));
  }

  /**
   * @brief Rotation around an arbitrary (not necessarily unit) axis.
   * @param axis  Axis; it is normalized internally so it may be any length.
   * @param angle Rotation angle in radians.
   */
  static constexpr auto rotation_axis(const Vec3<T>& axis, T angle) -> Mat3
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
    return Mat3(tx * a.x + c, txy - sz, txz + sy, txy + sz, ty * a.y + c, tyz - sx, txz - sy,
                tyz + sx, tz * a.z + c);
  }

  /** @brief Non-uniform scale matrix (per-axis factors). */
  static constexpr auto scaling(T sx, T sy, T sz) -> Mat3
  {
    return Mat3(sx, T(0), T(0), T(0), sy, T(0), T(0), T(0), sz);
  }

  /** @brief Approximate element-wise equality within `eps` (see #math::is_almost_equal). */
  [[nodiscard]] constexpr auto is_almost_equal(const Mat3& rhs, T eps = default_epsilon<T>()) const
    -> bool
  {
    for (int i = 0; i < 9; ++i)
      if (!math::is_almost_equal(data[i], rhs.data[i], eps))
        return false;
    return true;
  }
};

/** @brief 3x3 matrix alias using `float`. */
using Mat3f = Mat3<float>;
/** @brief 3x3 matrix alias using `double`. */
using Mat3d = Mat3<double>;

} // namespace obzcene::math
