#pragma once

/**
 * @file Picking.hpp
 * @brief Scene raycasting: find the nearest object under a ray (mouse
 *        picking, hitscan weapons, block placement).
 *
 * `obzcene::raycast` tests each scene object's transform-aware world AABB
 * (the same bounds used for frustum culling) and returns the closest hit,
 * including the entry face's outward axis — everything a sandbox needs to
 * both break and place blocks. Objects with empty/invalid bounds are
 * skipped, and an optional predicate can exclude objects (e.g. a viewmodel).
 *
 * @see Math/Ray.hpp (the intersection primitive), Viewer/Scene.hpp
 */

#include "obzcene/Math/Ray.hpp"
#include "obzcene/Viewer/Scene.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace obzcene
{

/** @brief The result of a successful scene raycast. */
struct RayHit
{
  /** @brief The hit object (handle stays valid across scene edits). */
  ObjectHandle object;
  /** @brief Ray parameter of the entry point (distance when the ray is unit). */
  float t{0.0F};
  /** @brief World-space entry point on the object's bounds. */
  math::Vec3f point;
  /** @brief Outward axis of the entry face (one component is ±1, rest 0). */
  math::Vec3i face{0, 0, 0};
};

/**
 * @brief Which axis of `box` the entry `point` lies on.
 * @param box   The box that was hit.
 * @param point Entry point on the box surface (ray origin + dir·t).
 * @param eps   Surface tolerance.
 * @return Outward unit axis of the closest face, or {0,0,0} when `point` is
 *         not on any face within `eps` (e.g. the ray started inside the box).
 */
[[nodiscard]] inline auto aabb_entry_face(const math::AABBf& box, const math::Vec3f& point,
                                          float eps = 1.0e-4F) -> math::Vec3i
{
  auto       face     = math::Vec3i{0, 0, 0};
  auto       best     = eps;
  const auto consider = [&](float distance, const math::Vec3i& axis)
  {
    if (distance < best)
    {
      best = distance;
      face = axis;
    }
  };
  consider(std::abs(point.x - box.min.x), {-1, 0, 0});
  consider(std::abs(point.x - box.max.x), {1, 0, 0});
  consider(std::abs(point.y - box.min.y), {0, -1, 0});
  consider(std::abs(point.y - box.max.y), {0, 1, 0});
  consider(std::abs(point.z - box.min.z), {0, 0, -1});
  consider(std::abs(point.z - box.max.z), {0, 0, 1});
  return face;
}

/**
 * @brief Find the nearest scene object hit by `ray`.
 * @param scene    Scene to test.
 * @param ray      Pick ray (unit direction recommended for meaningful `t`).
 * @param max_t    Farthest accepted entry distance.
 * @param include  Optional predicate; objects it rejects are skipped (null
 *                 handles and null models are always skipped).
 * @return The closest hit, or std::nullopt when nothing was within range.
 *
 * @note The test uses each object's transform-aware AABB, so hits land on
 *       bounding boxes, not exact triangles — refine further when a game
 *       needs sub-object precision (see examples/sandbox.cpp, which drills
 *       into the hit chunk's blocks).
 */
[[nodiscard]] inline auto raycast(const Scene& scene, const math::Rayf& ray, float max_t,
                                  bool (*include)(const ObjectHandle&) = nullptr)
  -> std::optional<RayHit>
{
  auto best    = max_t;
  auto best_ix = std::optional<usz>{};
  for (usz i = 0; i < scene.objects.size(); ++i)
  {
    const auto& object = scene.objects[i];
    if (object == nullptr || object->model == nullptr)
    {
      continue;
    }
    if (include != nullptr && !include(object))
    {
      continue;
    }
    const auto bounds = object->transformed_bounds(object->model->mesh.bounds);
    if (bounds.min.x > bounds.max.x)
    {
      continue; // empty/invalid bounds (e.g. a fully dug-out chunk)
    }
    auto t = 0.0F;
    if (ray.intersect_aabb(bounds, &t) && t < best)
    {
      best    = t;
      best_ix = i;
    }
  }
  if (!best_ix.has_value())
  {
    return std::nullopt;
  }
  const auto& object = scene.objects[*best_ix];
  const auto  point  = ray.origin + ray.direction * best;
  return RayHit{object, best, point,
                aabb_entry_face(object->transformed_bounds(object->model->mesh.bounds), point)};
}

/**
 * @brief Every object the ray crosses within `max_t`, nearest first.
 * @note Useful as a broad-phase before sub-object refinement: iterate the
 *       returned hits in order and stop at the first one your fine-grained
 *       test accepts (examples/sandbox.cpp drills into chunk hits this way).
 */
[[nodiscard]] inline auto raycast_all(const Scene& scene, const math::Rayf& ray, float max_t,
                                      bool (*include)(const ObjectHandle&) = nullptr)
  -> std::vector<RayHit>
{
  auto hits = std::vector<RayHit>{};
  for (const auto& object : scene.objects)
  {
    if (object == nullptr || object->model == nullptr)
    {
      continue;
    }
    if (include != nullptr && !include(object))
    {
      continue;
    }
    const auto bounds = object->transformed_bounds(object->model->mesh.bounds);
    if (bounds.min.x > bounds.max.x)
    {
      continue;
    }
    auto t = 0.0F;
    if (ray.intersect_aabb(bounds, &t) && t < max_t)
    {
      const auto point = ray.origin + ray.direction * t;
      hits.push_back(RayHit{object, t, point, aabb_entry_face(bounds, point)});
    }
  }
  std::sort(hits.begin(), hits.end(), [](const RayHit& a, const RayHit& b) { return a.t < b.t; });
  return hits;
}

} // namespace obzcene
