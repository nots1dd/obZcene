#pragma once

/**
 * @file Ray.hpp
 * @brief A ray with sphere/plane/box intersection tests.
 *
 * Rays are produced by obzcene::Camera::screen_to_ray (unprojecting a pixel or
 * NDC point) and by screen_to_ray_pixel; the intersection tests are the math
 * needed to turn those rays into pick queries. Aliases: #math::Rayf,
 * #math::Rayd.
 *
 * @see Camera (ray generation), Vec3, AABB (box test)
 */

#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cmath>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A ray defined by an origin and a travel direction.
 * @tparam T Scalar type (``float``/``double``).
 *
 * The `direction` is not normalized automatically; `intersect_*` work with any
 * non-zero direction as long as it is used consistently. Distances `t` returned
 * are measured in direction units.
 */
template <typename T> struct Ray
{
  /** @brief Start point of the ray. */
  Vec3<T> origin{};

  /** @brief Travel direction (defaults to +Z). */
  Vec3<T> direction{0, 0, 1};

  /** @brief Default ray at the origin pointing +Z. */
  constexpr Ray() = default;

  /** @brief Explicit origin + direction. */
  constexpr Ray(const Vec3<T>& origin_, const Vec3<T>& direction_)
      : origin(origin_), direction(direction_)
  {
  }

  /** @brief Point on the ray at parameter `t`: `origin + direction * t`. */
  [[nodiscard]] constexpr auto point_at(T t) const -> Vec3<T> { return origin + direction * t; }

  /**
   * @brief Ray/sphere intersection (closest hit).
   * @param center Sphere center, `radius` its radius.
   * @param t_out  Optional output: `t` of the closest, non-negative hit.
   * @return True when the ray hits the sphere at `t >= 0`.
   */
  [[nodiscard]] auto intersect_sphere(const Vec3<T>& center, T radius, T* t_out = nullptr) const
    -> bool
  {
    auto oc           = origin - center;
    auto a            = direction.dot(direction);
    auto half_b       = oc.dot(direction);
    auto c            = oc.dot(oc) - radius * radius;
    auto discriminant = half_b * half_b - a * c;
    if (discriminant < 0)
      return false;
    auto sqrtd = std::sqrt(discriminant);
    auto t1    = (-half_b - sqrtd) / a;
    auto t2    = (-half_b + sqrtd) / a;
    if (t2 < 0)
      return false;
    if (t1 < 0)
    {
      if (t_out)
        *t_out = t2;
      return true;
    }
    if (t_out)
      *t_out = t1;
    return true;
  }

  /**
   * @brief Ray/plane intersection.
   * @param normal Plane normal (any length; `d` must match the same scale).
   * @param d      Plane constant from `dot(normal, point) = d`.
   * @param t_out  Optional output: ray parameter at the hit point.
   * @return True when the ray hits the plane at `t >= 0` (parallel misses).
   */
  [[nodiscard]] auto intersect_plane(const Vec3<T>& normal, T d, T* t_out = nullptr) const -> bool
  {
    auto denom = direction.dot(normal);
    if (std::abs(denom) < default_epsilon<T>())
      return false;
    auto t = (d - origin.dot(normal)) / denom;
    if (t < 0)
      return false;
    if (t_out)
      *t_out = t;
    return true;
  }

  /**
   * @brief Slab-method ray/AABB intersection.
   * @param box        Axis-aligned box in min/max form.
   * @param t_min_out  Optional output: entry `t` of the hit interval.
   * @param t_max_out  Optional output: exit `t` of the hit interval.
   * @return True when the ray crosses the box at `t >= 0`.
   */
  [[nodiscard]] auto intersect_aabb(const AABB<T>& box, T* t_min_out = nullptr,
                                    T* t_max_out = nullptr) const -> bool
  {
    auto inv_dir = Vec3<T>{T(1) / direction.x, T(1) / direction.y, T(1) / direction.z};
    auto t0      = (box.min - origin) * inv_dir;
    auto t1      = (box.max - origin) * inv_dir;
    auto tmin    = min(t0, t1);
    auto tmax    = max(t0, t1);
    auto t_near  = std::max({tmin.x, tmin.y, tmin.z});
    auto t_far   = std::min({tmax.x, tmax.y, tmax.z});
    if (t_far < 0 || t_near > t_far)
      return false;
    if (t_min_out)
      *t_min_out = t_near;
    if (t_max_out)
      *t_max_out = t_far;
    return true;
  }
};

using Rayf = Ray<float>;
using Rayd = Ray<double>;

} // namespace obzcene::math
