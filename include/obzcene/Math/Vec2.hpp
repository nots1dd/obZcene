#pragma once

/**
 * @file Vec2.hpp
 * @brief 2D vector type with arithmetic, dot/cross and normalization helpers.
 *
 * The canonical scalar aliases are #math::Vec2f and #math::Vec2d; this file
 * also provides free-function flavours of the member API (dot, cross, length,
 * lerp, min/max/clamp) mirroring Vec3 and Vec4 so the whole family can be used
 * uniformly.
 *
 * @see Vec3 (positions/normals), Vec4 (clip-space), Common (epsilon helpers)
 */

#include "obzcene/Math/Common.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A generic 2-component vector.
 * @tparam T Scalar type (``float``/``double``/``int``).
 *
 * Used for 2D texture coordinates: obzcene::MeshVertex::texcoord,
 * rasterizer screen-space points and maths::barycentric (`Vec2`).
 */
template <typename T> struct Vec2
{
  /** @brief Scalar type stored in the vector. */
  using value_type = T;

  T x{};
  T y{};

  /** @brief Default-initialized zero vector. */
  constexpr Vec2() = default;

  /** @brief Component-wise constructor. */
  constexpr Vec2(T x_, T y_) : x(x_), y(y_) {}

  /** @brief Explicit constructor that fills both components with `s`. */
  constexpr explicit Vec2(T s) : x(s), y(s) {}

  /**
   * @brief Explicit cross-type conversion (e.g. Vec2d from Vec2f, or
   *        Vec2f from Vec2i from the integer screen-space rasterizer helpers).
   */
  template <typename U>
  constexpr explicit Vec2(const Vec2<U>& other)
      : x(static_cast<T>(other.x)), y(static_cast<T>(other.y))
  {
  }

  /** @brief Convert to another scalar type explicitly. */
  template <typename U> [[nodiscard]] constexpr auto cast_to() const -> Vec2<U>
  {
    return {static_cast<U>(x), static_cast<U>(y)};
  }

  /** @brief Zero vector (0,0). */
  static constexpr auto zero() -> Vec2 { return Vec2{}; }

  /** @brief Unit vector (1,1). */
  static constexpr auto one() -> Vec2 { return Vec2{T(1), T(1)}; }

  /** @brief Indexed access: `v[0]` is `x`, `v[1]` is `y` (debug-range-checked). */
  [[nodiscard]] constexpr auto operator[](std::size_t i) -> T&
  {
    assert(i < 2);
    return (i == 0) ? x : y;
  }

  [[nodiscard]] constexpr auto operator[](std::size_t i) const -> const T&
  {
    assert(i < 2);
    return (i == 0) ? x : y;
  }

  /** @brief The compound arithmetic operators below are the base of the free
   *         operator friends (which each forward into these). */
  constexpr auto operator+=(const Vec2& rhs) -> Vec2&
  {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }

  constexpr auto operator-=(const Vec2& rhs) -> Vec2&
  {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }

  constexpr auto operator*=(const Vec2& rhs) -> Vec2&
  {
    x *= rhs.x;
    y *= rhs.y;
    return *this;
  }

  constexpr auto operator/=(const Vec2& rhs) -> Vec2&
  {
    x /= rhs.x;
    y /= rhs.y;
    return *this;
  }

  constexpr auto operator*=(T s) -> Vec2&
  {
    x *= s;
    y *= s;
    return *this;
  }

  constexpr auto operator/=(T s) -> Vec2&
  {
    x /= s;
    y /= s;
    return *this;
  }

  friend constexpr auto operator+(Vec2 a, const Vec2& b) -> Vec2
  {
    a += b;
    return a;
  }

  friend constexpr auto operator-(Vec2 a, const Vec2& b) -> Vec2
  {
    a -= b;
    return a;
  }

  friend constexpr auto operator-(const Vec2& v) -> Vec2 { return Vec2{-v.x, -v.y}; }

  friend constexpr auto operator*(Vec2 a, const Vec2& b) -> Vec2
  {
    a *= b;
    return a;
  }

  friend constexpr auto operator*(Vec2 v, T s) -> Vec2
  {
    v *= s;
    return v;
  }

  friend constexpr auto operator*(T s, Vec2 v) -> Vec2
  {
    v *= s;
    return v;
  }

  friend constexpr auto operator/(Vec2 a, const Vec2& b) -> Vec2
  {
    a /= b;
    return a;
  }

  friend constexpr auto operator/(Vec2 v, T s) -> Vec2
  {
    v /= s;
    return v;
  }

  constexpr auto operator==(const Vec2& rhs) const -> bool { return x == rhs.x && y == rhs.y; }

  constexpr auto operator!=(const Vec2& rhs) const -> bool { return !(*this == rhs); }

  /** @brief Dot product `x*rhs.x + y*rhs.y`. */
  [[nodiscard]] constexpr auto dot(const Vec2& rhs) const -> T { return x * rhs.x + y * rhs.y; }

  /** @brief 2D scalar cross product `x*rhs.y - y*rhs.x` (signed). */
  [[nodiscard]] constexpr auto cross(const Vec2& rhs) const -> T { return x * rhs.y - y * rhs.x; }

  /** @brief Squared length (avoids the `sqrt` of #length). */
  [[nodiscard]] constexpr auto length_sq() const -> T { return dot(*this); }

  /**
   * @brief Euclidean length.
   * @tparam T Must be floating-point for `std::sqrt`.
   */
  [[nodiscard]] auto length() const -> T
  {
    static_assert(std::is_floating_point_v<T>, "length requires a floating-point Vec2<T>");
    return std::sqrt(length_sq());
  }

  /**
   * @brief Unit-length copy, or the zero vector when the input is degenerate.
   * @see math::normalized(world-space helper), obzcene::Light factory functions.
   */
  [[nodiscard]] auto normalized() const -> Vec2
  {
    static_assert(std::is_floating_point_v<T>, "normalized requires a floating-point Vec2<T>");
    const auto len = length();
    if (len > T(0))
      return *this / len;
    return Vec2{};
  }

  /** @brief Approximate element-wise equality within `eps` (see #math::is_almost_equal). */
  [[nodiscard]] constexpr auto is_almost_equal(const Vec2& rhs, T eps = default_epsilon<T>()) const
    -> bool
  {
    return math::is_almost_equal(x, rhs.x, eps) && math::is_almost_equal(y, rhs.y, eps);
  }
};

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2i = Vec2<int>;

/**
 * @name Free-function helpers
 * Non-member equivalents of the member API so callers can write
 * `math::dot(a, b)` / `math::normalized(v)` uniformly across vectors.
 * @{
 */

/** @brief Free-function dot product (see Vec2<T>::dot). */
template <typename T> [[nodiscard]] constexpr auto dot(const Vec2<T>& a, const Vec2<T>& b) -> T
{
  return a.dot(b);
}

/** @brief Free-function 2D cross product (see Vec2<T>::cross). */
template <typename T> [[nodiscard]] constexpr auto cross(const Vec2<T>& a, const Vec2<T>& b) -> T
{
  return a.cross(b);
}

/** @brief See Vec2<T>::length_sq. */
template <typename T> [[nodiscard]] constexpr auto length_sq(const Vec2<T>& v) -> T
{
  return v.length_sq();
}

/** @brief See Vec2<T>::length. */
template <typename T> [[nodiscard]] auto length(const Vec2<T>& v) -> T { return v.length(); }

/** @brief See Vec2<T>::normalized. */
template <typename T> [[nodiscard]] auto normalized(const Vec2<T>& v) -> Vec2<T>
{
  return v.normalized();
}

/** @brief Squared distance between two points `a` and `b`. */
template <typename T>
[[nodiscard]] constexpr auto distance_sq(const Vec2<T>& a, const Vec2<T>& b) -> T
{
  return (a - b).length_sq();
}

/** @brief Distance between two points `a` and `b`. */
template <typename T> [[nodiscard]] auto distance(const Vec2<T>& a, const Vec2<T>& b) -> T
{
  return (a - b).length();
}

/**
 * @brief Component-wise linear interpolation (see #math::lerp).
 * @note The rasterizer interpolates world/uv/normal per pixel with
 *       perspective-correct weights (see #raster_detail::perspective_blend)
 *       rather than plain lerp.
 */
template <typename T>
[[nodiscard]] constexpr auto lerp(const Vec2<T>& a, const Vec2<T>& b, T t) -> Vec2<T>
{
  return Vec2<T>{math::lerp(a.x, b.x, t), math::lerp(a.y, b.y, t)};
}

/** @brief Component-wise minimum (see #math::min). */
template <typename T>
[[nodiscard]] constexpr auto min(const Vec2<T>& a, const Vec2<T>& b) -> Vec2<T>
{
  return Vec2<T>{math::min(a.x, b.x), math::min(a.y, b.y)};
}

/** @brief Component-wise maximum (see #math::max). */
template <typename T>
[[nodiscard]] constexpr auto max(const Vec2<T>& a, const Vec2<T>& b) -> Vec2<T>
{
  return Vec2<T>{math::max(a.x, b.x), math::max(a.y, b.y)};
}

/** @brief Component-wise clamp (see #math::clamp). */
template <typename T>
[[nodiscard]] constexpr auto clamp(const Vec2<T>& v, const Vec2<T>& low, const Vec2<T>& high)
  -> Vec2<T>
{
  return Vec2<T>{math::clamp(v.x, low.x, high.x), math::clamp(v.y, low.y, high.y)};
}

/** @brief Component-wise clamp to a uniform [low, high] range. */
template <typename T> [[nodiscard]] constexpr auto clamp(const Vec2<T>& v, T low, T high) -> Vec2<T>
{
  return Vec2<T>{math::clamp(v.x, low, high), math::clamp(v.y, low, high)};
}

/** @} */

} // namespace obzcene::math