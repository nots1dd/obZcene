#pragma once

/**
 * @file Quaternion.hpp
 * @brief Quaternion rotations with conversion to matrices and interpolation.
 *
 * A unit quaternion describes an orientation without gimbal lock. Use
 * `to_mat3`/`to_mat4` to feed the renderer's transforms, or `slerp`/`nlerp`
 * for animating between orientations. Aliases: #math::Quatf, #math::Quatd.
 *
 * @see Mat3, Mat4 (conversion targets), Common (lerp/clamp helpers used by
 *      the interpolation functions)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Mat3.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cassert>
#include <cmath>
#include <type_traits>
#include <utility>

namespace obzcene::math
{

/**
 * @brief A quaternion `(w, x, y, z)`, w being the scalar part.
 * @tparam T Scalar type (``float``/``double``).
 *
 * Defaults to the identity quaternion (w=1). Rotating a vector is done with
 * `operator*(Vec3)`; conversion to the renderer's matrix types is available
 * through to_mat3/to_mat4.
 */
template <typename T> struct Quat
{
  T w{1};
  T x{0};
  T y{0};
  T z{0};

  /** @brief Identity quaternion by default. */
  constexpr Quat() = default;

  /** @brief Component-wise constructor `(w, x, y, z)`. */
  constexpr Quat(T w_, T x_, T y_, T z_) : w(w_), x(x_), y(y_), z(z_) {}

  /**
   * @brief Build a rotation quaternion of `angle` radians around `axis`.
   * @note `axis` does not need to be unit length; it is not normalized here
   *       (pre-normalize it when the axis is not already unit).
   */
  constexpr Quat(T angle, const Vec3<T>& axis)
  {
    const T half = angle * T(0.5);
    const T s    = std::sin(half);
    w            = std::cos(half);
    x            = axis.x * s;
    y            = axis.y * s;
    z            = axis.z * s;
  }

  /**
   * @brief Build a quaternion from roll/pitch/yaw (intrinsic order XYZ).
   * @param roll  Rotation around X.
   * @param pitch Rotation around Y.
   * @param yaw   Rotation around Z.
   * @note The inverse conversion exists via to_euler; be careful that repeated
   *       round-trips can flip sign (see is_almost_equal).
   */
  static constexpr auto from_euler(T roll, T pitch, T yaw) -> Quat
  {
    const T cr = std::cos(roll * T(0.5));
    const T sr = std::sin(roll * T(0.5));
    const T cp = std::cos(pitch * T(0.5));
    const T sp = std::sin(pitch * T(0.5));
    const T cy = std::cos(yaw * T(0.5));
    const T sy = std::sin(yaw * T(0.5));
    return Quat{cr * cp * cy + sr * sp * sy, sr * cp * cy - cr * sp * sy,
                cr * sp * cy + sr * cp * sy, cr * cp * sy - sr * sp * cy};
  }

  /** @brief Quaternion Hamilton product (composes rotations, right first). */
  constexpr auto operator*(const Quat& rhs) const -> Quat
  {
    return Quat{
      w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z, w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
      w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x, w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w};
  }

  /** @brief Rotate vector `v` by this quaternion (quaternion-vector product). */
  constexpr auto operator*(const Vec3<T>& v) const -> Vec3<T>
  {
    const Vec3<T> xyz{x, y, z};
    const Vec3<T> t = xyz.cross(v) * T(2);
    return v + xyz.cross(t) * T(1) + t * w;
  }

  constexpr auto operator==(const Quat& rhs) const -> bool
  {
    return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
  }

  constexpr auto operator!=(const Quat& rhs) const -> bool { return !(*this == rhs); }

  /** @brief Squared norm (avoids the `sqrt` of #length). */
  [[nodiscard]] constexpr auto length_sq() const -> T { return w * w + x * x + y * y + z * z; }

  /** @brief Euclidean norm. */
  [[nodiscard]] auto length() const -> T
  {
    static_assert(std::is_floating_point_v<T>, "length requires a floating-point type");
    return std::sqrt(length_sq());
  }

  /**
   * @brief Unit-length copy, or the identity quaternion for a zero input.
   * @note Keeps rotations numerically stable; call after slerp/nlerp.
   */
  [[nodiscard]] auto normalized() const -> Quat
  {
    static_assert(std::is_floating_point_v<T>, "normalized requires a floating-point type");
    const auto len = length();
    if (len > T(0))
    {
      const T inv = T(1) / len;
      return Quat{w * inv, x * inv, y * inv, z * inv};
    }
    return Quat{};
  }

  /** @brief Conjugate (negates the vector part), the inverse of a unit quaternion. */
  [[nodiscard]] constexpr auto conjugate() const -> Quat { return Quat{w, -x, -y, -z}; }

  /** @brief True inverse (conjugate / norm²); asserts on a zero quaternion. */
  [[nodiscard]] constexpr auto inverse() const -> Quat
  {
    const T lsq = length_sq();
    assert(lsq > T(0));
    const T inv = T(1) / lsq;
    return Quat{w * inv, -x * inv, -y * inv, -z * inv};
  }

  /** @brief Equivalent rotation as a 3x3 matrix (see Mat3). */
  [[nodiscard]] constexpr auto to_mat3() const -> Mat3<T>
  {
    const T xx = x * x;
    const T yy = y * y;
    const T zz = z * z;
    const T xy = x * y;
    const T xz = x * z;
    const T yz = y * z;
    const T wx = w * x;
    const T wy = w * y;
    const T wz = w * z;
    return Mat3<T>{T(1) - T(2) * (yy + zz), T(2) * (xy - wz),        T(2) * (xz + wy),
                   T(2) * (xy + wz),        T(1) - T(2) * (xx + zz), T(2) * (yz - wx),
                   T(2) * (xz - wy),        T(2) * (yz + wx),        T(1) - T(2) * (xx + yy)};
  }

  /** @brief Equivalent rotation as a homogeneous 4x4 matrix (see Mat4). */
  [[nodiscard]] constexpr auto to_mat4() const -> Mat4<T>
  {
    const Mat3<T> m3 = to_mat3();
    return Mat4<T>{m3.data[0], m3.data[1], m3.data[2], T(0),       m3.data[3], m3.data[4],
                   m3.data[5], T(0),       m3.data[6], m3.data[7], m3.data[8], T(0),
                   T(0),       T(0),       T(0),       T(1)};
  }

  /** @brief Axis and angle (radians) representation; axis defaults to +X when `w`≈±1. */
  [[nodiscard]] auto to_axis_angle() const -> std::pair<Vec3<T>, T>
  {
    static_assert(std::is_floating_point_v<T>, "to_axis_angle requires a floating-point type");
    const T angle = T(2) * std::acos(clamp(w, T(-1), T(1)));
    const T s     = std::sqrt(T(1) - w * w);
    if (s > default_epsilon<T>())
    {
      return {Vec3<T>{x, y, z} / s, angle};
    }
    return {Vec3<T>{T(1), T(0), T(0)}, angle};
  }

  /**
   * @brief Roll/pitch/yaw (radians) as a Vec3 — inverse of from_euler.
   * @return (roll, pitch, yaw) = (X, Y, Z) angles.
   */
  [[nodiscard]] auto to_euler() const -> Vec3<T>
  {
    static_assert(std::is_floating_point_v<T>, "to_euler requires a floating-point type");
    const T roll  = std::atan2(T(2) * (w * x + y * z), T(1) - T(2) * (x * x + y * y));
    const T pitch = std::asin(clamp(T(2) * (w * y - z * x), T(-1), T(1)));
    const T yaw   = std::atan2(T(2) * (w * z + x * y), T(1) - T(2) * (y * y + z * z));
    return {roll, pitch, yaw};
  }

  /**
   * @brief Spherical interpolation along the shortest arc between `a` and `b`.
   * @param t Blend factor in [0,1]; 0 returns `a`, 1 returns `b`.
   * @note Falls back to #nlerp when the two quaternions are nearly parallel to
   *       avoid the numerical instability of sin(θ)≈0, and flips `b` to always
   *       take the short way around the hypersphere.
   */
  static constexpr auto slerp(const Quat& a, const Quat& b, T t) -> Quat
  {
    static_assert(std::is_floating_point_v<T>, "slerp requires a floating-point type");
    T    dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    Quat end = b;
    if (dot < T(0))
    {
      end = Quat{-b.w, -b.x, -b.y, -b.z};
      dot = -dot;
    }
    if (dot > T(0.9995))
    {
      return nlerp(a, end, t);
    }
    const T theta     = std::acos(clamp(dot, T(-1), T(1)));
    const T sin_theta = std::sin(theta);
    const T wa        = std::sin((T(1) - t) * theta) / sin_theta;
    const T wb        = std::sin(t * theta) / sin_theta;
    return Quat{a.w * wa + end.w * wb, a.x * wa + end.x * wb, a.y * wa + end.y * wb,
                a.z * wa + end.z * wb};
  }

  /**
   * @brief Normalized linear interpolation between `a` and `b`.
   * @note Cheaper than #slerp but not constant angular speed; used as a
   *       fallback inside slerp itself.
   */
  static constexpr auto nlerp(const Quat& a, const Quat& b, T t) -> Quat
  {
    static_assert(std::is_floating_point_v<T>, "nlerp requires a floating-point type");
    Quat end = b;
    if (a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z < T(0))
    {
      end = Quat{-b.w, -b.x, -b.y, -b.z};
    }
    const Quat result{math::lerp(a.w, end.w, t), math::lerp(a.x, end.x, t),
                      math::lerp(a.y, end.y, t), math::lerp(a.z, end.z, t)};
    return result.normalized();
  }

  /**
   * @brief Approximate equality treating `q` and `-q` as the same orientation.
   * @note Quat and -Quat rotate identically, so the comparison first flips
   *       this quaternion to share `rhs`'s sign when they are antipodal.
   */
  [[nodiscard]] constexpr auto is_almost_equal(const Quat& rhs, T eps = default_epsilon<T>()) const
    -> bool
  {
    const Quat a =
      (w * rhs.w + x * rhs.x + y * rhs.y + z * rhs.z) < T(0) ? Quat{-w, -x, -y, -z} : *this;
    return math::is_almost_equal(a.w, rhs.w, eps) && math::is_almost_equal(a.x, rhs.x, eps) &&
           math::is_almost_equal(a.y, rhs.y, eps) && math::is_almost_equal(a.z, rhs.z, eps);
  }
};

/** @brief Quaternion alias using `float`. */
using Quatf = Quat<float>;
/** @brief Quaternion alias using `double`. */
using Quatd = Quat<double>;

} // namespace obzcene::math
