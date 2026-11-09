#pragma once

/**
 * @file Vec4.hpp
 * @brief 4-component vector type, primarily used for clip-space coordinates.
 *
 * Clip-space vertices carry a homogeneous `w` that is used for the perspective
 * divide and the per-pixel perspective-correct interpolation
 * (see obzcene::rasterize and #math::raster_detail::perspective_blend).
 * Aliases: #math::Vec4f, #math::Vec4d.
 *
 * @see Vec3 (world space), Mat4 (transform matrices acting on Vec4),
 *      obzcene::world_to_clip
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A generic 4-component vector.
 * @tparam T Scalar type (``float``/``double``).
 */
template <typename T> struct Vec4
{
  /** @brief Scalar type stored in the vector. */
  using value_type = T;

  T x{};
  T y{};
  T z{};
  T w{};

  /** @brief Default-initialized zero vector. */
  constexpr Vec4() = default;

  /** @brief Component-wise constructor. */
  constexpr Vec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

  /** @brief Explicit constructor that fills all components with `s`. */
  constexpr explicit Vec4(T s) : x(s), y(s), z(s), w(s) {}

  /**
   * @brief Explicit cross-type conversion (e.g. Vec4d from Vec4f).
   */
  template <typename U>
  constexpr explicit Vec4(const Vec4<U>& other)
      : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)),
        w(static_cast<T>(other.w))
  {
  }

  /** @brief Convert to another scalar type explicitly. */
  template <typename U> [[nodiscard]] constexpr auto cast_to() const -> Vec4<U>
  {
    return {static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w)};
  }

  /** @brief Zero vector (0,0,0,0). */
  static constexpr auto zero() -> Vec4 { return Vec4{}; }

  /** @brief Unit vector (1,1,1,1). */
  static constexpr auto one() -> Vec4 { return Vec4{T(1), T(1), T(1), T(1)}; }

  /** @brief Indexed access: `v[0]`=x .. `v[3]`=w (debug-range-checked). */
  [[nodiscard]] constexpr auto operator[](std::size_t i) -> T&
  {
    assert(i < 4);
    return (i == 0) ? x : ((i == 1) ? y : ((i == 2) ? z : w));
  }

  [[nodiscard]] constexpr auto operator[](std::size_t i) const -> const T&
  {
    assert(i < 4);
    return (i == 0) ? x : ((i == 1) ? y : ((i == 2) ? z : w));
  }

  constexpr auto operator+=(const Vec4& rhs) -> Vec4&
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
  }

  constexpr auto operator-=(const Vec4& rhs) -> Vec4&
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    w -= rhs.w;
    return *this;
  }

  constexpr auto operator*=(const Vec4& rhs) -> Vec4&
  {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    w *= rhs.w;
    return *this;
  }

  constexpr auto operator/=(const Vec4& rhs) -> Vec4&
  {
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    w /= rhs.w;
    return *this;
  }

  constexpr auto operator*=(T s) -> Vec4&
  {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
  }

  constexpr auto operator/=(T s) -> Vec4&
  {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
    return *this;
  }

  friend constexpr auto operator+(Vec4 a, const Vec4& b) -> Vec4
  {
    a += b;
    return a;
  }

  friend constexpr auto operator-(Vec4 a, const Vec4& b) -> Vec4
  {
    a -= b;
    return a;
  }

  friend constexpr auto operator-(const Vec4& v) -> Vec4 { return Vec4{-v.x, -v.y, -v.z, -v.w}; }

  friend constexpr auto operator*(Vec4 a, const Vec4& b) -> Vec4
  {
    a *= b;
    return a;
  }

  friend constexpr auto operator*(Vec4 v, T s) -> Vec4
  {
    v *= s;
    return v;
  }

  friend constexpr auto operator*(T s, Vec4 v) -> Vec4
  {
    v *= s;
    return v;
  }

  friend constexpr auto operator/(Vec4 a, const Vec4& b) -> Vec4
  {
    a /= b;
    return a;
  }

  friend constexpr auto operator/(Vec4 v, T s) -> Vec4
  {
    v /= s;
    return v;
  }

  constexpr auto operator==(const Vec4& rhs) const -> bool
  {
    return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
  }

  constexpr auto operator!=(const Vec4& rhs) const -> bool { return !(*this == rhs); }

  /** @brief Dot product `x*x + y*y + z*z + w*w` style (see #math::dot). */
  [[nodiscard]] constexpr auto dot(const Vec4& rhs) const -> T
  {
    return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
  }

  /** @brief Squared length (avoids the `sqrt` of #length). */
  [[nodiscard]] constexpr auto length_sq() const -> T { return dot(*this); }

  /**
   * @brief Euclidean length.
   * @tparam T Must be floating-point for `std::sqrt`.
   */
  [[nodiscard]] auto length() const -> T
  {
    static_assert(std::is_floating_point_v<T>, "length requires a floating-point Vec4<T>");
    return std::sqrt(length_sq());
  }

  /** @brief Unit-length copy, or the zero vector when the input is degenerate. */
  [[nodiscard]] auto normalized() const -> Vec4
  {
    static_assert(std::is_floating_point_v<T>, "normalized requires a floating-point Vec4<T>");
    const auto len = length();
    if (len > T(0))
      return *this / len;
    return Vec4{};
  }

  /** @brief Drop `w`, producing a Vec3 (world-space data lives in x/y/z). */
  [[nodiscard]] constexpr auto xyz() const -> Vec3<T> { return Vec3<T>{x, y, z}; }

  /** @brief Drop `z` and `w`, producing a Vec2. */
  [[nodiscard]] constexpr auto xy() const -> Vec2<T> { return Vec2<T>{x, y}; }

  /** @brief Approximate element-wise equality within `eps` (see #math::is_almost_equal). */
  [[nodiscard]] constexpr auto is_almost_equal(const Vec4& rhs, T eps = default_epsilon<T>()) const
    -> bool
  {
    return math::is_almost_equal(x, rhs.x, eps) && math::is_almost_equal(y, rhs.y, eps) &&
           math::is_almost_equal(z, rhs.z, eps) && math::is_almost_equal(w, rhs.w, eps);
  }
};

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;

/**
 * @name Free-function helpers
 * Non-member equivalents of the member API so callers can write
 * `math::dot(a, b)` / `math::normalized(v)` uniformly across vectors.
 * @{
 */

/** @brief Free-function dot product (see Vec4<T>::dot). */
template <typename T> [[nodiscard]] constexpr auto dot(const Vec4<T>& a, const Vec4<T>& b) -> T
{
  return a.dot(b);
}

/** @brief See Vec4<T>::length_sq. */
template <typename T> [[nodiscard]] constexpr auto length_sq(const Vec4<T>& v) -> T
{
  return v.length_sq();
}

/** @brief See Vec4<T>::length. */
template <typename T> [[nodiscard]] auto length(const Vec4<T>& v) -> T { return v.length(); }

/** @brief See Vec4<T>::normalized. */
template <typename T> [[nodiscard]] auto normalized(const Vec4<T>& v) -> Vec4<T>
{
  return v.normalized();
}

/** @brief Squared distance between two points `a` and `b`. */
template <typename T>
[[nodiscard]] constexpr auto distance_sq(const Vec4<T>& a, const Vec4<T>& b) -> T
{
  return (a - b).length_sq();
}

/** @brief Distance between two points `a` and `b`. */
template <typename T> [[nodiscard]] auto distance(const Vec4<T>& a, const Vec4<T>& b) -> T
{
  return (a - b).length();
}

/** @brief Component-wise linear interpolation (see #math::lerp). */
template <typename T>
[[nodiscard]] constexpr auto lerp(const Vec4<T>& a, const Vec4<T>& b, T t) -> Vec4<T>
{
  return Vec4<T>{math::lerp(a.x, b.x, t), math::lerp(a.y, b.y, t), math::lerp(a.z, b.z, t),
                 math::lerp(a.w, b.w, t)};
}

/** @brief Component-wise minimum (see #math::min). */
template <typename T>
[[nodiscard]] constexpr auto min(const Vec4<T>& a, const Vec4<T>& b) -> Vec4<T>
{
  return Vec4<T>{math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z),
                 math::min(a.w, b.w)};
}

/** @brief Component-wise maximum (see #math::max). */
template <typename T>
[[nodiscard]] constexpr auto max(const Vec4<T>& a, const Vec4<T>& b) -> Vec4<T>
{
  return Vec4<T>{math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z),
                 math::max(a.w, b.w)};
}

/** @brief Component-wise clamp (see #math::clamp). */
template <typename T>
[[nodiscard]] constexpr auto clamp(const Vec4<T>& v, const Vec4<T>& low, const Vec4<T>& high)
  -> Vec4<T>
{
  return Vec4<T>{math::clamp(v.x, low.x, high.x), math::clamp(v.y, low.y, high.y),
                 math::clamp(v.z, low.z, high.z), math::clamp(v.w, low.w, high.w)};
}

/** @brief Component-wise clamp to a uniform [low, high] range. */
template <typename T> [[nodiscard]] constexpr auto clamp(const Vec4<T>& v, T low, T high) -> Vec4<T>
{
  return Vec4<T>{math::clamp(v.x, low, high), math::clamp(v.y, low, high),
                 math::clamp(v.z, low, high), math::clamp(v.w, low, high)};
}

/** @} */

} // namespace obzcene::math