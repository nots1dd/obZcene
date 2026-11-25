#pragma once

/**
 * @file Batch.hpp
 * @brief Static mesh batching: merge many transformed copies of a mesh into
 *        one draw-ready Mesh.
 *
 * The rasterizer pays a fixed setup cost per rendered object (frustum test,
 * vertex-pool preparation, worker fan-out), so scenes built from thousands of
 * small objects — voxel terrain, scattered props — spend their frame budget on
 * overhead instead of pixels. Batching bakes those instances into a single
 * mesh up front: one object, one setup, one parallel fan-out per frame.
 *
 * - #obzcene::append_mesh appends one transformed copy to an existing mesh
 *   (general accumulator; materials are appended and remapped).
 * - #obzcene::bake_instances merges N transforms of the SAME source mesh,
 *   sharing one material table (the common voxel/prop case).
 *
 * The result is a plain Mesh with world-space vertices: wrap it in a Model at
 * the origin and it renders like any other object. Baked meshes are static —
 * to change one instance, re-bake the batch (keep batches small enough, e.g.
 * terrain chunks, that a re-bake is cheap).
 *
 * @see Renderer/Mesh.hpp (the Mesh being merged), Viewer/Scene.hpp (Model)
 */

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/Mat3.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Renderer/Mesh.hpp"

#include <vector>

namespace obzcene
{

namespace batch_detail
{

/**
 * @brief Append `src` geometry to `dst` under `transform`, remapping material
 *        indices by `material_offset` (does NOT touch the material table).
 */
inline auto append_geometry(Mesh& dst, const Mesh& src, const math::Mat4f& transform,
                            ui32 material_offset) -> void
{
  const auto vertex_offset = narrow_cast<ui32>(dst.vertices.size());
  const auto normal_matrix = transform.upper3().inverse().transpose();

  for (const auto& vertex : src.vertices)
  {
    const auto p =
      transform * math::Vec4f{vertex.position.x, vertex.position.y, vertex.position.z, 1.0F};
    MeshVertex out;
    out.position = math::Vec3f{p.x, p.y, p.z};
    out.normal   = normal_matrix * vertex.normal;
    out.texcoord = vertex.texcoord;
    dst.bounds.expand(out.position);
    dst.vertices.push_back(out);
  }
  for (const auto& tri : src.triangles)
  {
    dst.triangles.push_back(MeshTriangle{tri.a + vertex_offset, tri.b + vertex_offset,
                                         tri.c + vertex_offset,
                                         tri.material_index + material_offset});
  }
  dst.object_count += src.object_count;
}

} // namespace batch_detail

/**
 * @brief Append a transformed copy of `src` to `dst`.
 * @param dst        Accumulator mesh; grows bounds/vertices/triangles/materials.
 * @param src        Mesh to append (unmodified).
 * @param transform  Model-space -> dst-space transform applied per vertex
 *                   (normals use the inverse-transpose, so non-uniform scale
 *                   stays correct).
 * @note `src` materials are appended to `dst` and its triangles remapped, so
 *       mixing different source meshes in one batch keeps their materials.
 *       Bounds accumulate from `dst.bounds`' current value — start from
 *       `math::AABBf::empty()` when building a fresh mesh by hand.
 */
inline auto append_mesh(Mesh& dst, const Mesh& src, const math::Mat4f& transform) -> void
{
  if (src.triangles.empty())
  {
    return;
  }
  const auto material_offset = narrow_cast<ui32>(dst.materials.size());
  batch_detail::append_geometry(dst, src, transform, material_offset);
  dst.materials.insert(dst.materials.end(), src.materials.begin(), src.materials.end());
  if (dst.materials.empty())
  {
    dst.materials.push_back(Material{});
  }
}

/**
 * @brief Bake many transformed instances of one mesh into a single mesh.
 * @param src         The shared source mesh (e.g. one voxel block).
 * @param transforms  One model transform per instance.
 * @return A mesh containing every instance in dst/world space, sharing a
 *         single copy of `src`'s material table; empty transforms yield an
 *         empty mesh with the source materials.
 *
 * @note Vertices/triangles are pre-reserved for the full instance count, so
 *       baking is one allocation plus a straight-line copy loop.
 */
[[nodiscard]] inline auto bake_instances(const Mesh&                     src,
                                         const std::vector<math::Mat4f>& transforms) -> Mesh
{
  Mesh out;
  out.bounds    = math::AABBf::empty();
  out.materials = src.materials;
  if (out.materials.empty())
  {
    out.materials.push_back(Material{});
  }
  out.vertices.reserve(src.vertices.size() * transforms.size());
  out.triangles.reserve(src.triangles.size() * transforms.size());
  for (const auto& transform : transforms)
  {
    batch_detail::append_geometry(out, src, transform, 0);
  }
  return out;
}

} // namespace obzcene
