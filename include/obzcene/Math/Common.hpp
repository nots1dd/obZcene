#pragma once

/**
 * @file Common.hpp
 * @brief Scalar math constants and helpers shared by the rest of the math module.
 *
 * Everything here is `constexpr` (or `constexpr`-compatible) so it can be used
 * at compile time. This header is the foundation for all of Math/ and is also
 * pulled in indirectly by obzcene::Camera, obzcene::Light and obzcene::Mesh,
 * which is why it only depends on the standard library.
 *
 * @see obzcene/Math/Math.hpp (aggregate that includes every math sub-header)
 */

#include <algorithm>
#include <cassert>
#include <limits>
#include <numbers>
#include <type_traits>

/**
 * @namespace math
 * @brief Generic, header-only math (vectors, matrices, quaternions, geometry).
 *
 * All types are templates over a scalar `T` (`float`/`double`) and provide
 * convenience aliases such as #math::Vec3f and #math::Mat4f. Used pervasively
 * by the renderer: camera, lights, shading, rasterizer and texture sampling.
 */
namespace obzcene::math
{

/** @brief The circle constant π for `T`. */
template <typename T> inline constexpr T pi = std::numbers::pi_v<T>;

/** @brief Two times π (a full circle in radians). */
template <typename T> inline constexpr T two_pi = T(2) * std::numbers::pi_v<T>;

/** @brief Half of π (a right angle in radians). */
template <typename T> inline constexpr T half_pi = std::numbers::pi_v<T> / T(2);

/**
 * @brief Default comparison tolerance for approximate equality checks.
 * @tparam T Scalar type (must be floating-point).
 * @return Eight ULPs of the type's epsilon, scaled to the operands.
 * @see #math::is_almost_equal
 */
template <typename T> constexpr auto default_epsilon() -> T
{
  static_assert(std::is_floating_point_v<T>, "default_epsilon requires a floating-point type");
  return std::numeric_limits<T>::epsilon() * T(8);
}

/** @brief Absolute value, branch-free for integer types. */
template <typename T> constexpr auto abs(T value) -> T { return value < T(0) ? -value : value; }

/** @brief Component-wise-friendly element minimum (`std::min`). */
template <typename T> constexpr auto min(T a, T b) -> T { return std::min(a, b); }

/** @brief Component-wise-friendly element maximum (`std::max`). */
template <typename T> constexpr auto max(T a, T b) -> T { return std::max(a, b); }

/**
 * @brief Clamp `value` into the inclusive range [low, high].
 * @note Debug builds assert that `low <= high`.
 */
template <typename T> constexpr auto clamp(T value, T low, T high) -> T
{
  assert(low <= high);
  if (value < low)
    return low;
  if (value > high)
    return high;
  return value;
}

/**
 * @brief Linear interpolation between `a` and `b` by `t` (t=0 gives a, t=1 gives b).
 * @see math::Vec2f/Vec3f/Vec4f `lerp` overloads, and math::Quat `nlerp`/`slerp`.
 */
template <typename T> constexpr auto lerp(T a, T b, T t) -> T { return a + (b - a) * t; }

/**
 * @brief Convert degrees to radians.
 * @tparam T Scalar type (must be floating-point). Used by
 *         obzcene::Camera (fov/pitch) and obzcene::Light (spot cutoff defaults).
 */
template <typename T> constexpr auto deg_to_rad(T degrees) -> T
{
  static_assert(std::is_floating_point_v<T>, "deg_to_rad requires a floating-point type");
  return degrees * (pi<T> / T(180));
}

/** @brief Convert radians to degrees. */
template <typename T> constexpr auto rad_to_deg(T radians) -> T
{
  static_assert(std::is_floating_point_v<T>, "rad_to_deg requires a floating-point type");
  return radians * (T(180) / pi<T>);
}

/**
 * @brief Approximate floating-point equality within a relative tolerance.
 * @param a,b  Values being compared.
 * @param eps  Tolerance scale relative to the magnitudes of `a`/`b`
 *             (defaults to #math::default_epsilon).
 * @return True when `|a-b| <= eps * max(|a|,|b|,1)`.
 * @see math::Vec2<T>::is_almost_equal, math::Quat<T>::is_almost_equal,
 *      math::Mat4<T>::is_almost_equal
 */
template <typename T> constexpr auto is_almost_equal(T a, T b, T eps = default_epsilon<T>()) -> bool
{
  static_assert(std::is_floating_point_v<T>, "is_almost_equal requires a floating-point type");
  const auto diff  = abs(a - b);
  const auto scale = max(abs(a), max(abs(b), T(1)));
  return diff <= eps * scale;
}

} // namespace obzcene::math