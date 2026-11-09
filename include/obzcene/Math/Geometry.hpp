#pragma once

/**
 * @file Geometry.hpp
 * @brief Higher-order geometry helpers: a view frustum and triangle utilities.
 *
 * The math::Frustum is the 6-plane view volume produced by
 * obzcene::Camera::frustum() from the view-projection matrix; the barycentric /
 * point-in-triangle / area helpers back 2D triangle tests used by the raster
 * edge functions conceptually (the actual per-pixel cost lives in
 * obzcene::rasterize). `reflect`/`refract`/`face_forward` are the classic
 * vector utilities available to shading code.
 *
 * @see Plane, AABB, Camera (frustum provider)
 */

#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Plane.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cmath>
#include <type_traits>

namespace obzcene::math
{

/**
 * @brief A view frustum: six planes (left, right, bottom, top, near, far).
 * @tparam T Scalar type (``float``/``double``).
 *
 * All planes are expected to be normalized; inside means `signed_distance >= 0`
 * for every plane. Built by extraction from a view-projection matrix in
 * obzcene::Camera::frustum().
 */
template <typename T> struct Frustum
{
  /** @brief The six bounding planes, all pointing inward. */
  Plane<T> planes[6];

  /** @brief Default-constructed (uninitialized) frustum. */
  constexpr Frustum() = default;

  /** @brief Explicit plane order: left, right, bottom, top, near, far. */
  constexpr Frustum(const Plane<T>& left, const Plane<T>& right, const Plane<T>& bottom,
                    const Plane<T>& top, const Plane<T>& near_p, const Plane<T>& far_p)
      : planes{left, right, bottom, top, near_p, far_p}
  {
  }

  /** @brief True when `point` is inside all six planes. */
  [[nodiscard]] constexpr auto contains_point(const Vec3<T>& point) const -> bool
  {
    for (int i = 0; i < 6; ++i)
    {
      if (planes[i].signed_distance(point) < T(0))
        return false;
    }
    return true;
  }

  /**
   * @brief True when a sphere (center, radius) intersects the frustum.
   * @note A sphere is rejected as soon as any plane is behind it by more than
   *       its radius; this is the classic cheap bounding-sphere test.
   */
  [[nodiscard]] constexpr auto contains_sphere(const Vec3<T>& center, T radius) const -> bool
  {
    for (int i = 0; i < 6; ++i)
    {
      if (planes[i].signed_distance(center) < -radius)
        return false;
    }
    return true;
  }

  /**
   * @brief True when an AABB intersects the frustum.
   * @note Uses the "positive vertex" trick: the box corner furthest along each
   *       plane's normal, which is sufficient for a conservative overlap test.
   */
  [[nodiscard]] constexpr auto contains_aabb(const AABB<T>& box) const -> bool
  {
    for (int i = 0; i < 6; ++i)
    {
      auto n = planes[i].normal;
      auto positive_vertex =
        Vec3<T>{n.x >= T(0) ? box.max.x : box.min.x, n.y >= T(0) ? box.max.y : box.min.y,
                n.z >= T(0) ? box.max.z : box.min.z};
      if (planes[i].signed_distance(positive_vertex) < T(0))
        return false;
    }
    return true;
  }
};

/** @brief Frustum alias using `float`. */
using Frustumf = Frustum<float>;
/** @brief Frustum alias using `double`. */
using Frustumd = Frustum<double>;

/**
 * @brief Barycentric coordinates of point `p` inside triangle `a,b,c`.
 * @return (u, v, w) with `p = u*a + v*b + w*c`; returns `(-1,-1,-1)` when the
 *         triangle is degenerate.
 */
template <typename T>
[[nodiscard]] constexpr auto barycentric(const Vec2<T>& p, const Vec2<T>& a, const Vec2<T>& b,
                                         const Vec2<T>& c) -> Vec3<T>
{
  auto v0    = b - a;
  auto v1    = c - a;
  auto v2    = p - a;
  auto d00   = v0.dot(v0);
  auto d01   = v0.dot(v1);
  auto d11   = v1.dot(v1);
  auto d20   = v2.dot(v0);
  auto d21   = v2.dot(v1);
  auto denom = d00 * d11 - d01 * d01;
  if (std::abs(denom) < default_epsilon<T>())
    return Vec3<T>{-1, -1, -1};
  auto v = (d11 * d20 - d01 * d21) / denom;
  auto w = (d00 * d21 - d01 * d20) / denom;
  auto u = T(1) - v - w;
  return Vec3<T>{u, v, w};
}

/**
 * @brief Point-in-triangle test (inclusive of the edges, within epsilon).
 * @note The rasterizer uses an equivalent half-plane edge test instead of this
 *       to stay branch-light per pixel; see obzcene::rasterize.
 */
template <typename T>
[[nodiscard]] constexpr auto point_in_triangle(const Vec2<T>& p, const Vec2<T>& a, const Vec2<T>& b,
                                               const Vec2<T>& c) -> bool
{
  auto bary = barycentric(p, a, b, c);
  return bary.x >= -default_epsilon<T>() && bary.y >= -default_epsilon<T>() &&
         bary.z >= -default_epsilon<T>();
}

/** @brief Area of the 2D triangle `a,b,c` (always non-negative). */
template <typename T>
[[nodiscard]] constexpr auto triangle_area(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c)
  -> T
{
  return std::abs((b - a).cross(c - a)) * T(0.5);
}

/**
 * @brief Orient `n` toward the viewer: returns `n` when it already points
 *        approximately along `view_dir` (`dot >= 0`), otherwise `-n`.
 * @note This is the inverse convention of the GLSL `faceforward`; useful when
 *       a computed normal needs to face the light or camera.
 */
template <typename T>
[[nodiscard]] auto face_forward(const Vec3<T>& n, const Vec3<T>& view_dir) -> Vec3<T>
{
  if (n.dot(view_dir) >= T(0))
    return n;
  return -n;
}

/**
 * @brief Reflect `incident` off a surface with `normal`.
 * @return `incident - 2*dot(normal, incident)*normal`, i.e. the mirror of the
 *         incoming direction across the plane of `normal`.
 */
template <typename T>
[[nodiscard]] constexpr auto reflect(const Vec3<T>& incident, const Vec3<T>& normal) -> Vec3<T>
{
  return incident - T(2) * normal.dot(incident) * normal;
}

/**
 * @brief Refract `incident` across the boundary with ratio `eta`.
 * @param eta  Ratio of refractive indices (n₁/n₂).
 * @return The refracted direction, or the zero vector on total internal
 *         reflection.
 */
template <typename T>
[[nodiscard]] constexpr auto refract(const Vec3<T>& incident, const Vec3<T>& normal, T eta)
  -> Vec3<T>
{
  auto cos_i  = -normal.dot(incident);
  auto sin2_i = T(1) - cos_i * cos_i;
  auto sin2_t = sin2_i / (eta * eta);
  if (sin2_t > T(1))
    return Vec3<T>::zero();
  auto cos_t = std::sqrt(T(1) - sin2_t);
  return incident * (T(1) / eta) + normal * (cos_i / eta - cos_t);
}

} // namespace obzcene::math
