#pragma once

/**
 * @file Vec3.hpp
 * @brief 3D vector type used for world-space positions, normals and colors.
 *
 * This is the most widely used vector in the project: it appears in
 * obzcene::Camera (position/orientation), obzcene::Light (position, direction,
 * color), obzcene::MeshVertex (position/normal), materials (RGB), and in the
 * per-pixel shader input. Aliases: #math::Vec3f, #math::Vec3d.
 *
 * @see Vec2 (2D texcoords), Vec4 (clip space), Common (scalar helpers)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec2.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A generic 3-component vector.
 * @tparam T Scalar type (``float``/``double``).
 */
template <typename T> struct Vec3
{
  /** @brief Scalar type stored in the vector. */
  using value_type = T;

  T x{};
  T y{};
  T z{};

  /** @brief Default-initialized zero vector. */
  constexpr Vec3() = default;

  /** @brief Component-wise constructor. */
  constexpr Vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

  /** @brief Explicit constructor that fills all components with `s`. */
  constexpr explicit Vec3(T s) : x(s), y(s), z(s) {}

  /**
   * @brief Explicit cross-type conversion (e.g. Vec3d from Vec3f).
   */
  template <typename U>
  constexpr explicit Vec3(const Vec3<U>& other)
      : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z))
  {
  }

  /** @brief Convert to another scalar type explicitly. */
  template <typename U> [[nodiscard]] constexpr auto cast_to() const -> Vec3<U>
  {
    return {static_cast<U>(x), static_cast<U>(y), static_cast<U>(z)};
  }

  /** @brief Zero vector (0,0,0). */
  static constexpr auto zero() -> Vec3 { return Vec3{}; }

  /** @brief Unit vector (1,1,1). */
  static constexpr auto one() -> Vec3 { return Vec3{T(1), T(1), T(1)}; }

  /** @brief Indexed access: `v[0]`=x, `v[1]`=y, `v[2]`=z (debug-range-checked). */
  [[nodiscard]] constexpr auto operator[](std::size_t i) -> T&
  {
    assert(i < 3);
    return (i == 0) ? x : ((i == 1) ? y : z);
  }

  [[nodiscard]] constexpr auto operator[](std::size_t i) const -> const T&
  {
    assert(i < 3);
    return (i == 0) ? x : ((i == 1) ? y : z);
  }

  constexpr auto operator+=(const Vec3& rhs) -> Vec3&
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }

  constexpr auto operator-=(const Vec3& rhs) -> Vec3&
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }

  constexpr auto operator*=(const Vec3& rhs) -> Vec3&
  {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
  }

  constexpr auto operator/=(const Vec3& rhs) -> Vec3&
  {
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    return *this;
  }

  constexpr auto operator*=(T s) -> Vec3&
  {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  constexpr auto operator/=(T s) -> Vec3&
  {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }

  friend constexpr auto operator+(Vec3 a, const Vec3& b) -> Vec3
  {
    a += b;
    return a;
  }

  friend constexpr auto operator-(Vec3 a, const Vec3& b) -> Vec3
  {
    a -= b;
    return a;
  }

  friend constexpr auto operator-(const Vec3& v) -> Vec3 { return Vec3{-v.x, -v.y, -v.z}; }

  friend constexpr auto operator*(Vec3 a, const Vec3& b) -> Vec3
  {
    a *= b;
    return a;
  }

  friend constexpr auto operator*(Vec3 v, T s) -> Vec3
  {
    v *= s;
    return v;
  }

  friend constexpr auto operator*(T s, Vec3 v) -> Vec3
  {
    v *= s;
    return v;
  }

  friend constexpr auto operator/(Vec3 a, const Vec3& b) -> Vec3
  {
    a /= b;
    return a;
  }

  friend constexpr auto operator/(Vec3 v, T s) -> Vec3
  {
    v /= s;
    return v;
  }

  constexpr auto operator==(const Vec3& rhs) const -> bool
  {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }

  constexpr auto operator!=(const Vec3& rhs) const -> bool { return !(*this == rhs); }

  /** @brief Dot product `x*rhs.x + y*rhs.y + z*rhs.z`. */
  [[nodiscard]] constexpr auto dot(const Vec3& rhs) const -> T
  {
    return x * rhs.x + y * rhs.y + z * rhs.z;
  }

  /**
   * @brief 3D cross product (perpendicular to both operands).
   * @see obzcene::Plane (built from `(b-a) x (c-a)`), and #math::Vec2::cross
   *      (the rasterizer's screen-space edge function uses the 2D variant).
   */
  [[nodiscard]] constexpr auto cross(const Vec3& rhs) const -> Vec3
  {
    return Vec3{y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
  }

  /** @brief Squared length (avoids the `sqrt` of #length). */
  [[nodiscard]] constexpr auto length_sq() const -> T { return dot(*this); }

  /**
   * @brief Euclidean length.
   * @tparam T Must be floating-point for `std::sqrt`.
   */
  [[nodiscard]] auto length() const -> T
  {
    static_assert(std::is_floating_point_v<T>, "length requires a floating-point Vec3<T>");
    return std::sqrt(length_sq());
  }

  /**
   * @brief Unit-length copy, or the zero vector when the input is degenerate.
   * @see math::normalized, obzcene::Camera::forward/right/up, obzcene::Light
   *      factory functions, obzcene::Shader (view direction, normals).
   */
  [[nodiscard]] auto normalized() const -> Vec3
  {
    static_assert(std::is_floating_point_v<T>, "normalized requires a floating-point Vec3<T>");
    const auto len = length();
    if (len > T(0))
      return *this / len;
    return Vec3{};
  }

  /** @brief Drop the Z component, producing a Vec2 (e.g. for top-down math). */
  [[nodiscard]] constexpr auto xy() const -> Vec2<T> { return Vec2<T>{x, y}; }

  /** @brief Approximate element-wise equality within `eps` (see #math::is_almost_equal). */
  [[nodiscard]] constexpr auto is_almost_equal(const Vec3& rhs, T eps = default_epsilon<T>()) const
    -> bool
  {
    return math::is_almost_equal(x, rhs.x, eps) && math::is_almost_equal(y, rhs.y, eps) &&
           math::is_almost_equal(z, rhs.z, eps);
  }
};

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

/**
 * @name Free-function helpers
 * Non-member equivalents of the member API so callers can write
 * `math::dot(a, b)` / `math::normalized(v)` uniformly across vectors.
 * @{
 */

/** @brief Free-function dot product (see Vec3<T>::dot). */
template <typename T> [[nodiscard]] constexpr auto dot(const Vec3<T>& a, const Vec3<T>& b) -> T
{
  return a.dot(b);
}

/** @brief Free-function cross product (see Vec3<T>::cross). */
template <typename T>
[[nodiscard]] constexpr auto cross(const Vec3<T>& a, const Vec3<T>& b) -> Vec3<T>
{
  return a.cross(b);
}

/** @brief See Vec3<T>::length_sq. */
template <typename T> [[nodiscard]] constexpr auto length_sq(const Vec3<T>& v) -> T
{
  return v.length_sq();
}

/** @brief See Vec3<T>::length. */
template <typename T> [[nodiscard]] auto length(const Vec3<T>& v) -> T { return v.length(); }

/** @brief See Vec3<T>::normalized. */
template <typename T> [[nodiscard]] auto normalized(const Vec3<T>& v) -> Vec3<T>
{
  return v.normalized();
}

/** @brief Squared distance between two points `a` and `b`. */
template <typename T>
[[nodiscard]] constexpr auto distance_sq(const Vec3<T>& a, const Vec3<T>& b) -> T
{
  return (a - b).length_sq();
}

/** @brief Distance between two points `a` and `b`. */
template <typename T> [[nodiscard]] auto distance(const Vec3<T>& a, const Vec3<T>& b) -> T
{
  return (a - b).length();
}

/**
 * @brief Component-wise linear interpolation (see #math::lerp).
 * @note The rasterizer interpolates world position, normal and UV per pixel
 *       between triangle corners with perspective-correct weights
 *       (#raster_detail::perspective_blend).
 */
template <typename T>
[[nodiscard]] constexpr auto lerp(const Vec3<T>& a, const Vec3<T>& b, T t) -> Vec3<T>
{
  return Vec3<T>{math::lerp(a.x, b.x, t), math::lerp(a.y, b.y, t), math::lerp(a.z, b.z, t)};
}

/** @brief Component-wise minimum (see #math::min). */
template <typename T>
[[nodiscard]] constexpr auto min(const Vec3<T>& a, const Vec3<T>& b) -> Vec3<T>
{
  return Vec3<T>{math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z)};
}

/** @brief Component-wise maximum (see #math::max). */
template <typename T>
[[nodiscard]] constexpr auto max(const Vec3<T>& a, const Vec3<T>& b) -> Vec3<T>
{
  return Vec3<T>{math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z)};
}

/** @brief Component-wise clamp (see #math::clamp). */
template <typename T>
[[nodiscard]] constexpr auto clamp(const Vec3<T>& v, const Vec3<T>& low, const Vec3<T>& high)
  -> Vec3<T>
{
  return Vec3<T>{math::clamp(v.x, low.x, high.x), math::clamp(v.y, low.y, high.y),
                 math::clamp(v.z, low.z, high.z)};
}

/** @brief Component-wise clamp to a uniform [low, high] range. */
template <typename T> [[nodiscard]] constexpr auto clamp(const Vec3<T>& v, T low, T high) -> Vec3<T>
{
  return Vec3<T>{math::clamp(v.x, low, high), math::clamp(v.y, low, high),
                 math::clamp(v.z, low, high)};
}

/** @} */

} // namespace obzcene::math