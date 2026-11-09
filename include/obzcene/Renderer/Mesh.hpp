#pragma once

/**
 * @file Mesh.hpp
 * @brief Renderable, indexed triangle soup plus the OBJ-to-Mesh builder.
 *
 * ObjData (Core/Types.hpp) is a faithful mirror of the file's flat vertex
 * lists; #obzcene::build_mesh converts it into the tightly-packed vertices
 * + index list the rasterizer actually eats (identical corners are shared
 * through a vertex cache, missing normals get a computed face normal).
 * Triangle `material_index` points into `materials`, which by construction
 * aligns with `obj_data.materials` (and the texture list loaded in
 * src/Texture/TextureLoader.cpp).
 *
 * @see Core/Types.hpp (ObjData/Material), File/Obj/Parser/ObjParser.hpp
 *      (parse_obj), Rasterizer.hpp (consumer)
 */

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace obzcene
{

/** @brief One corner of a triangle: copies of the OBJ vertex attributes. */
struct MeshVertex
{
  math::Vec3f position;
  math::Vec3f normal;
  math::Vec2f texcoord;
};

/** @brief A triangle as three vertex indices plus its material. */
struct MeshTriangle
{
  /** @brief First corner index into Mesh::vertices. */
  ui32 a;
  /** @brief Second corner index into Mesh::vertices. */
  ui32 b;
  /** @brief Third corner index into Mesh::vertices. */
  ui32 c;
  /** @brief Index into Mesh::materials (see Core/Types.hpp Material). */
  ui32 material_index;
};

/**
 * @brief The render-ready geometry for one model.
 *
 * `vertices`/`triangles` are fully expanded (see build_mesh), `materials`
 * mirrors the parsed OBJ materials, and `bounds` is the precomputed AABB used
 * by the App to frame the camera.
 */
struct Mesh
{
  /** @brief Unique vertex pool; triangles reference it by index (shared corners). */
  std::vector<MeshVertex> vertices;
  /** @brief Indexed triangles; each references three vertices and a material. */
  std::vector<MeshTriangle> triangles;
  /** @brief Material table referenced by triangle `material_index`. */
  std::vector<Material> materials;
  /** @brief Model-space bounding box computed at build time. */
  math::AABBf bounds;
  /** @brief Number of OBJ objects (`o`) baked into this mesh. */
  usz object_count{0};
};

namespace mesh_detail
{

/**
 * @brief Vertex-cache key: the OBJ index triple that defines one corner.
 *
 * Corners of faces without vertex normals share a computed per-face normal,
 * so their key uses `normals_count + face_index` instead of a normal index to
 * keep them distinct from corners of other faces.
 */
struct VertexKey
{
  i32 position;
  i32 texcoord;
  i32 normal;

  auto operator==(const VertexKey&) const -> bool = default;
};

/** @brief boost-style hash combine over the three key fields. */
struct VertexKeyHash
{
  auto operator()(const VertexKey& key) const -> std::size_t
  {
    const auto h    = [](i32 v) { return std::hash<i32>{}(v); };
    auto       seed = h(key.position);
    seed ^= h(key.texcoord) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= h(key.normal) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

} // namespace mesh_detail

/**
 * @brief Convert parsed OBJ data into a rasterizable Mesh.
 * @param data  Output of `obzcene::parse_obj`.
 * @return A Mesh with materials copied from `data` (a default Material is
 *         appended when the file declared none), per-object triangles, and
 *         bounds = `data.bounds()`.
 *
 * @note Polygons are fan-triangulated around corner 0; degenerate triangles
 *       are skipped. Identical corners are shared through a vertex cache
 *       keyed on the OBJ index triple, so the vertex pool holds each unique
 *       corner once instead of three copies per triangle. When a face lacks
 *       per-vertex normals a single face normal is computed from one half of
 *       the Newell formula; when a face lacks texcoords those are zeroed.
 */
[[nodiscard]] inline auto build_mesh(const ObjData& data) -> Mesh
{
  Mesh mesh;
  mesh.materials = data.materials;
  if (mesh.materials.empty())
  {
    mesh.materials.push_back(Material{});
  }
  mesh.bounds       = data.bounds();
  mesh.object_count = data.objects.size();

  std::unordered_map<mesh_detail::VertexKey, ui32, mesh_detail::VertexKeyHash> cache;
  usz                                                                          face_index = 0;

  for (const auto& object : data.objects)
  {
    for (const auto& face : object.faces)
    {
      const auto corner_count = face.vertices.size();
      if (corner_count < 3)
      {
        continue;
      }
      ++face_index;

      math::Vec3f face_normal{0.0F};
      auto        have_face_normal = true;
      for (const auto& corner : face.vertices)
      {
        if (corner.normal < 0)
        {
          have_face_normal = false;
          break;
        }
      }
      if (!have_face_normal)
      {
        for (usz i = 0; i < corner_count; ++i)
        {
          const auto& p = data.positions[narrow_cast<usz>(face.vertices[i].position)];
          const auto& q =
            data.positions[narrow_cast<usz>(face.vertices[(i + 1) % corner_count].position)];
          face_normal.x += (p.y - q.y) * (p.z + q.z);
          face_normal.y += (p.z - q.z) * (p.x + q.x);
          face_normal.z += (p.x - q.x) * (p.y + q.y);
        }
        face_normal = face_normal.length_sq() > 1.0e-18F ? math::normalized(face_normal)
                                                         : math::Vec3f{0.0F, 1.0F, 0.0F};
      }

      // Corners of a face without vertex normals share that face's computed
      // normal, so the cache key must not merge them with other faces.
      const auto computed_normal_key =
        narrow_cast<i32>(data.normals.size()) + narrow_cast<i32>(face_index);

      const auto corner_index = [&](std::size_t i) -> ui32
      {
        const auto&                  ref = face.vertices[i];
        const mesh_detail::VertexKey key{ref.position, ref.texcoord,
                                         ref.normal >= 0 ? ref.normal : computed_normal_key};
        const auto [it, inserted] = cache.try_emplace(key, narrow_cast<ui32>(mesh.vertices.size()));
        if (inserted)
        {
          MeshVertex vertex;
          vertex.position = data.positions[narrow_cast<usz>(ref.position)];
          vertex.texcoord =
            ref.texcoord >= 0 ? data.texcoords[narrow_cast<usz>(ref.texcoord)] : math::Vec2f{0.0F};
          vertex.normal =
            ref.normal >= 0 ? data.normals[narrow_cast<usz>(ref.normal)] : face_normal;
          mesh.vertices.push_back(vertex);
        }
        return it->second;
      };

      const auto& first_position = data.positions[narrow_cast<usz>(face.vertices[0].position)];
      for (std::size_t k = 1; k + 1 < corner_count; ++k)
      {
        const auto& second_position = data.positions[narrow_cast<usz>(face.vertices[k].position)];
        const auto& third_position =
          data.positions[narrow_cast<usz>(face.vertices[k + 1].position)];
        const auto edge_a = second_position - first_position;
        const auto edge_b = third_position - first_position;
        if (math::cross(edge_a, edge_b).length_sq() < 1.0e-18F)
        {
          continue;
        }
        mesh.triangles.push_back(
          MeshTriangle{corner_index(0), corner_index(k), corner_index(k + 1), face.material_index});
      }
    }
  }
  return mesh;
}

} // namespace obzcene