#pragma once

/**
 * @file Interpolation.hpp
 * @brief Smooth scalar/vector interpolation helpers.
 *
 * Provides Hermite-style easing (smoothstep/smootherstep) and Catmull-Rom
 * spline evaluation for scalars, Vec2 and Vec3. These are the building blocks
 * for (future) camera/animation paths; the ubiquitous per-frame interpolation
 * in the renderer itself is the plain `math::lerp` family in Common/Vec3.
 *
 * @see Quaternion (slerp/nlerp build on math::lerp), Common
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cmath>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief Smoothstep easing: `3t²-2t³` over the [edge0, edge1] span.
 * @param edge0,edge1  Interval bounds; values outside are clamped to 0/1.
 * @param x            Input value to ease.
 */
template <typename T> constexpr auto smoothstep(T edge0, T edge1, T x) -> T
{
  auto t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
  return t * t * (T(3) - T(2) * t);
}

/**
 * @brief Smootherstep easing: `t⁵(6t²-15t+10)`, C2-continuous derivative.
 * @param edge0,edge1  Interval bounds; values outside are clamped to 0/1.
 * @param x            Input value to ease.
 */
template <typename T> constexpr auto smootherstep(T edge0, T edge1, T x) -> T
{
  auto t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
  return t * t * t * (t * (t * T(6) - T(15)) + T(10));
}

/**
 * @brief Catmull-Rom spline through control points `p1`/`p2`.
 * @param p0,p3  Neighbouring control points shaping the tangents.
 * @param t      Position along the segment in [0,1].
 */
template <typename T> constexpr auto catmull_rom(T p0, T p1, T p2, T p3, T t) -> T
{
  T t2 = t * t;
  T t3 = t2 * t;
  return T(0.5) * ((T(2) * p1) + (-p0 + p2) * t + (T(2) * p0 - T(5) * p1 + T(4) * p2 - p3) * t2 +
                   (-p0 + T(3) * p1 - T(3) * p2 + p3) * t3);
}

/** @brief Component-wise Catmull-Rom for Vec2 (see scalar overload). */
template <typename T>
constexpr auto catmull_rom(const Vec2<T>& p0, const Vec2<T>& p1, const Vec2<T>& p2,
                           const Vec2<T>& p3, T t) -> Vec2<T>
{
  return Vec2<T>{catmull_rom(p0.x, p1.x, p2.x, p3.x, t), catmull_rom(p0.y, p1.y, p2.y, p3.y, t)};
}

/** @brief Component-wise Catmull-Rom for Vec3 (see scalar overload). */
template <typename T>
constexpr auto catmull_rom(const Vec3<T>& p0, const Vec3<T>& p1, const Vec3<T>& p2,
                           const Vec3<T>& p3, T t) -> Vec3<T>
{
  return Vec3<T>{catmull_rom(p0.x, p1.x, p2.x, p3.x, t), catmull_rom(p0.y, p1.y, p2.y, p3.y, t),
                 catmull_rom(p0.z, p1.z, p2.z, p3.z, t)};
}

} // namespace obzcene::math
