#pragma once

/**
 * @file Plane.hpp
 * @brief Infinite plane in Hessian normal form.
 *
 * A plane is a unit (or at least non-zero) `normal` plus an offset `distance`
 * such that the plane equation is `dot(normal, point) - distance = 0`. This is
 * consumed by math::Frustum (six planes bounding the view volume) which the
 * obzcene::Camera::frustum() method derives from the view-projection matrix.
 * Aliases: #math::Planef, #math::Planed.
 *
 * @see Frustum, Camera (frustum extraction)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cmath>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A plane defined by `normal` and `distance`.
 * @tparam T Scalar type (``float``/``double``).
 */
template <typename T> struct Plane
{
  /** @brief Plane normal (does not need to be unit until #signed_distance is scaled). */
  Vec3<T> normal{0, 1, 0};
  /** @brief Signed distance of the origin from the plane along `normal`. */
  T distance{0};

  /** @brief Default plane: the XZ plane at the origin, normal +Y. */
  constexpr Plane() = default;

  /** @brief Plane through `normal` with the given signed `distance` offset. */
  constexpr Plane(const Vec3<T>& normal_, T distance_) : normal(normal_), distance(distance_) {}

  /**
   * @brief Plane passing through points `a`, `b` and `c`.
   * @note Counter-clockwise winding yields `normal` toward the viewer.
   */
  constexpr Plane(const Vec3<T>& a, const Vec3<T>& b, const Vec3<T>& c)
      : normal((b - a).cross(c - a).normalized()), distance(normal.dot(a))
  {
  }

  /**
   * @brief Signed distance of `point` to the plane (positive in front).
   * @note Correct only when `normal` is unit; see #normalized.
   * @see math::Frustum::contains_point / contains_sphere, which rely on this.
   */
  [[nodiscard]] constexpr auto signed_distance(const Vec3<T>& point) const -> T
  {
    return normal.dot(point) - distance;
  }

  /** @brief A copy with unit `normal` and `distance` rescaled accordingly. */
  [[nodiscard]] constexpr auto normalized() const -> Plane
  {
    return Plane{normal.normalized(), distance / normal.length()};
  }

  /** @brief Approximate equality of normal and distance (see #math::is_almost_equal). */
  constexpr auto is_almost_equal(const Plane& rhs, T eps = default_epsilon<T>()) const -> bool
  {
    return normal.is_almost_equal(rhs.normal, eps) &&
           math::is_almost_equal(distance, rhs.distance, eps);
  }
};

/** @brief Plane alias using `float`. */
using Planef = Plane<float>;
/** @brief Plane alias using `double`. */
using Planed = Plane<double>;

} // namespace obzcene::math
