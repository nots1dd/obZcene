#pragma once

/**
 * @file AABB.hpp
 * @brief Axis-aligned bounding box in *min/max* form.
 *
 * Bounding boxes are used for geometry preview/framing: obzcene::Mesh stores a
 * whole-model bounds built from the parsed OBJ (see obzcene::Mesh::bounds and
 * obzcene::ObjData::bounds), and obzcene::App::compute_scene_view frames the
 * camera around it. Also consumed by the math::Frustum containment tests.
 * Aliases: #math::AABBf, #math::AABBd.
 *
 * @see Vec3, Frustum (containment), Mesh/ObjData (model bounds)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <limits>

namespace obzcene::math
{

/**
 * @brief Axis-aligned bounding box represented by its min and max corners.
 * @tparam T Scalar type (``float``/``double``).
 */
template <typename T> struct AABB
{
  Vec3<T> min{};
  Vec3<T> max{};

  /** @brief Default-constructed empty (all-zero) box. */
  constexpr AABB() = default;

  /** @brief Box spanning `min_` to `max_`. */
  constexpr AABB(const Vec3<T>& min_, const Vec3<T>& max_) : min(min_), max(max_) {}

  /**
   * @brief An inverted, infinitely-large box useful for accumulating points.
   * @see AABB::expand (grows the box; starting from empty() makes it exact).
   */
  static constexpr auto empty() -> AABB
  {
    return AABB{Vec3<T>{std::numeric_limits<T>::max(), std::numeric_limits<T>::max(),
                        std::numeric_limits<T>::max()},
                Vec3<T>{std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest(),
                        std::numeric_limits<T>::lowest()}};
  }

  /** @brief Box centered at `center` and half-extended by `extent`. */
  static constexpr auto from_center_extent(const Vec3<T>& center, const Vec3<T>& extent) -> AABB
  {
    return AABB{center - extent, center + extent};
  }

  /** @brief Mid-point of the box. */
  [[nodiscard]] constexpr auto center() const -> Vec3<T> { return (min + max) / T(2); }

  /** @brief Half the box diagonal, one component per axis. */
  [[nodiscard]] constexpr auto extents() const -> Vec3<T> { return (max - min) / T(2); }

  /** @brief Full extent per axis (`max - min`). */
  [[nodiscard]] constexpr auto size() const -> Vec3<T> { return max - min; }

  /** @brief Surface area of the box (GJK/SAH metrics, tests). */
  [[nodiscard]] constexpr auto surface_area() const -> T
  {
    auto s  = size();
    auto dx = s.x;
    auto dy = s.y;
    auto dz = s.z;
    return T(2) * (dx * dy + dx * dz + dy * dz);
  }

  /** @brief Interior volume. */
  [[nodiscard]] constexpr auto volume() const -> T
  {
    auto s = size();
    return s.x * s.y * s.z;
  }

  /** @brief True when `point` lies inside the box, inclusive of the boundary. */
  [[nodiscard]] constexpr auto contains_point(const Vec3<T>& point) const -> bool
  {
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
  }

  /** @brief True when `other` is fully contained in this box. */
  [[nodiscard]] constexpr auto contains_aabb(const AABB& other) const -> bool
  {
    return other.min.x >= min.x && other.min.y >= min.y && other.min.z >= min.z &&
           other.max.x <= max.x && other.max.y <= max.y && other.max.z <= max.z;
  }

  /** @brief True when the two boxes overlap in all three axes (touching counts). */
  [[nodiscard]] constexpr auto intersects(const AABB& other) const -> bool
  {
    if (other.max.x < min.x || other.min.x > max.x)
      return false;
    if (other.max.y < min.y || other.min.y > max.y)
      return false;
    if (other.max.z < min.z || other.min.z > max.z)
      return false;
    return true;
  }

  /**
   * @brief Grow the box to include `point`; returns `*this` for chaining.
   * @see AABB::empty as the natural starting box.
   */
  constexpr auto expand(const Vec3<T>& point) -> AABB&
  {
    min = math::min(min, point);
    max = math::max(max, point);
    return *this;
  }

  /** @brief Grow the box to include another box. */
  constexpr auto expand(const AABB& other) -> AABB&
  {
    min = math::min(min, other.min);
    max = math::max(max, other.max);
    return *this;
  }

  /** @brief A new box covering both operands (does not mutate `*this`). */
  [[nodiscard]] constexpr auto union_with(const AABB& other) const -> AABB
  {
    auto result = *this;
    result.expand(other);
    return result;
  }

  /** @brief Approximate equality of both corners (see #math::is_almost_equal). */
  constexpr auto is_almost_equal(const AABB& rhs, T eps = default_epsilon<T>()) const -> bool
  {
    return min.is_almost_equal(rhs.min, eps) && max.is_almost_equal(rhs.max, eps);
  }
};

/** @brief AABB alias using `float`. */
using AABBf = AABB<float>;
/** @brief AABB alias using `double`. */
using AABBd = AABB<double>;

} // namespace obzcene::math
