#pragma once

/**
 * @file SceneObject.hpp
 * @brief A renderable scene object: shared model plus a world transform.
 *
 * obZcene scenes are flat lists of SceneObjects. An object references its
 * geometry through a std::shared_ptr so several objects can instance the same
 * model (loaded once via obzcene::Scene::load_model), and carries a full TRS
 * transform (translation, euler rotation, per-axis scale) applied per-vertex
 * by the rasterizer through the matrix from #model_matrix.
 *
 * @see Viewer/Scene.hpp (the scene container), Renderer/Rasterizer.hpp
 *      (the consumer of the model matrix)
 */

#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace obzcene
{

/** @brief Forward declaration; the full type lives in Viewer/Scene.hpp. */
struct Model;

/**
 * @brief One placed instance in a scene.
 *
 * Rotation is stored as euler angles in radians, applied X (pitch) then Y
 * (yaw) then Z (roll) — i.e. the model matrix is T·Ry·Rx·Rz·S.
 */
struct SceneObject
{
  /** @brief The shared model (never null for a loaded object). */
  std::shared_ptr<const Model> model;
  /** @brief World-space translation of the model's origin. */
  math::Vec3f position{0.0F};
  /** @brief Euler rotation in radians: x = pitch, y = yaw, z = roll. */
  math::Vec3f rotation{0.0F};
  /** @brief Per-axis scale factor (1 = original size). */
  math::Vec3f scale{1.0F, 1.0F, 1.0F};
  /** @brief Display name (defaults to the source file stem). */
  std::string name;
  /** @brief Path the model was loaded from (used when saving the scene). */
  std::filesystem::path source_path;

  /**
   * @brief Model matrix: translate(position) · Ry(yaw) · Rx(pitch) · Rz(roll)
   *        · scale.
   *
   * Composed so that rotation and scale apply about the mesh's own origin
   * before it is moved to #position, which is the natural "place this here"
   * pivot.
   */
  [[nodiscard]] auto model_matrix() const -> math::Mat4f
  {
    return math::Mat4f::translation(position.x, position.y, position.z) *
           math::Mat4f::rotation_y(rotation.y) * math::Mat4f::rotation_x(rotation.x) *
           math::Mat4f::rotation_z(rotation.z) * math::Mat4f::scaling(scale.x, scale.y, scale.z);
  }

  /**
   * @brief Normal matrix: inverse-transpose of the model's 3x3 block.
   *
   * Required for correctness with non-uniform scale; for pure rotation or
   * uniform scale this equals the rotation (the shader normalizes anyway).
   */
  [[nodiscard]] auto normal_matrix() const -> math::Mat3f
  {
    return model_matrix().upper3().inverse().transpose();
  }

  /**
   * @brief World-space AABB of the given mesh bounds under this transform
   *        (all 8 corners transformed, then re-enclosed).
   */
  [[nodiscard]] auto transformed_bounds(const math::AABBf& local) const -> math::AABBf
  {
    const auto box   = model_matrix();
    auto       out   = math::AABBf::empty();
    const auto xform = [&](float x, float y, float z)
    {
      const auto p = box * math::Vec4f{x, y, z, 1.0F};
      out.expand({p.x, p.y, p.z});
    };
    const auto mn = local.min;
    const auto mx = local.max;
    xform(mn.x, mn.y, mn.z);
    xform(mx.x, mn.y, mn.z);
    xform(mn.x, mx.y, mn.z);
    xform(mx.x, mx.y, mn.z);
    xform(mn.x, mn.y, mx.z);
    xform(mx.x, mn.y, mx.z);
    xform(mn.x, mx.y, mx.z);
    xform(mx.x, mx.y, mx.z);
    return out;
  }
};

} // namespace obzcene
