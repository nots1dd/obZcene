#pragma once

/**
 * @file Types.hpp
 * @brief Shared fixed-width aliases and the raw asset data structures.
 *
 * The aliases (`ui8`, `i32`, `usz`, ...) keep the codebase independent of
 * platform-specific `int` widths. The `Obj*` types are the intermediate
 * representation produced by the Wavefront OBJ/MTL parsers and consumed by
 * obzcene::build_mesh() to produce a renderable obzcene::Mesh.
 *
 * @see File/Obj/Parser/ObjParser.hpp (producer),
 *      Renderer/Mesh.hpp (consumer / build_mesh),
 *      Renderer/Color.hpp (ui8 color components)
 */

#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"

#include <cstdint>
#include <string>
#include <vector>

/**
 * @namespace obzcene
 * @brief Root namespace of the whole renderer.
 *
 * Sub-namespaces: `math` (vectors/matrices/geometry), `app` (entry + scene
 * assembly), `overlay` (HUD), `core` (debug backtrace) and `utils`. Everything
 * the engine ships lives under `include/obzcene`.
 */
namespace obzcene
{

using ui8  = std::uint8_t;
using ui16 = std::uint16_t;
using ui32 = std::uint32_t;
using ui64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using ull = unsigned long long;
using ll  = long long;

using usz = std::size_t;

/**
 * @brief A Wavefront MTL material definition.
 *
 * Field names follow the MTL keywords: Ka/Kd/Ks/Ke/Tf/Ns/Ni/d/illum and
 * `map_Kd`/`map_Ks`/`map_Ka`/`map_bump`. Parsed by `obzcene::parse_mtl()` and
 * handed to the obzcene::shade_fragment lighting model.
 *
 * @see File/Mtl/Parser/MtlParser.hpp, Renderer/Shader.hpp
 */
struct Material
{
  std::string name;
  /** @brief Ambient reflectivity (Mtl `Ka`); scales the scene ambient term. */
  math::Vec3f ambient{0.1F, 0.1F, 0.1F};
  /** @brief Diffuse reflectivity (Mtl `Kd`); the base color of the surface. */
  math::Vec3f diffuse{0.8F, 0.8F, 0.8F};
  /** @brief Specular reflectivity (Mtl `Ks`); zero disables specular highlights. */
  math::Vec3f specular{1.0F, 1.0F, 1.0F};
  /** @brief Emissive color (Mtl `Ke`); added to the shaded result unlit. */
  math::Vec3f emissive{0.0F, 0.0F, 0.0F};
  /** @brief Transmission filter (Mtl `Tf`); parsed but unused by the shader. */
  math::Vec3f transmission{1.0F, 1.0F, 1.0F};
  /** @brief Specular exponent (Mtl `Ns`), fed to obzcene::blinn_phong. */
  float shininess{32.0F};
  /** @brief Index of refraction (Mtl `Ni`); parsed but unused by the shader. */
  float ior{1.0F};
  /** @brief Opacity (Mtl `d` / `Tr`); parsed but unused by the shader (no alpha blending). */
  float opacity{1.0F};
  /** @brief MTL illumination model number; parsed but not interpreted. */
  int illum{2};
  /** @brief Diffuse map (Mtl `map_Kd`); the only map the shader samples. */
  std::string diffuse_texture;
  /** @brief Specular map (Mtl `map_Ks`); parsed but unused by the shader. */
  std::string specular_texture;
  /** @brief Normal map (Mtl `map_bump`/`bump`/`map_Kn`); parsed but unused by the shader. */
  std::string normal_texture;
  /** @brief Ambient map (Mtl `map_Ka`); parsed but unused by the shader. */
  std::string ambient_texture;
};

/**
 * @brief One corner of an OBJ face (`v/vt/vn` indices).
 *
 * Indices are 1-based or negative (relative) in the file; the parser resolves
 * them to 0-based, absolute indices via `obzcene::detail::resolve_index`.
 */
struct ObjVertexRef
{
  /** @brief 0-based index into `ObjData::positions`; -1 when absent. */
  i32 position{-1};
  /** @brief 0-based index into `ObjData::texcoords`; -1 when absent. */
  i32 texcoord{-1};
  /** @brief 0-based index into `ObjData::normals`; -1 when absent. */
  i32 normal{-1};
};

/**
 * @brief A polygon face of an OBJ object.
 */
struct ObjFace
{
  /** @brief Face corners (3 for a triangle, more for an n-gon). */
  std::vector<ObjVertexRef> vertices;
  /** @brief Index of the `usemtl` material at the time the face was parsed. */
  ui32 material_index{0};
};

/**
 * @brief A named group of faces inside an OBJ (`o`/`g` statements).
 */
struct ObjObject
{
  std::string          name;
  std::vector<ObjFace> faces;
};

/**
 * @brief The complete result of `obzcene::parse_obj()`.
 *
 * Retains the flat vertex lists of the file plus the face-adjacency the
 * builder (obzcene::build_mesh) needs to create a renderable Mesh.
 */
struct ObjData
{
  std::vector<math::Vec3f> positions;
  std::vector<math::Vec2f> texcoords;
  std::vector<math::Vec3f> normals;
  std::vector<Material>    materials;
  std::vector<ObjObject>   objects;
  /** @brief Name of the `.mtl` library, as referenced by `mtllib`. */
  std::string material_lib;

  /**
   * @brief Bounding box covering every position.
   * @return `AABBf::empty()` when there are no positions, otherwise the
   *         results of expanding the first position by the rest.
   * @see obzcene::Mesh::bounds, obzcene::App::compute_scene_view (fits the
   *      camera around this box).
   */
  [[nodiscard]] auto bounds() const -> math::AABBf
  {
    if (positions.empty())
    {
      return math::AABBf::empty();
    }
    auto box = math::AABBf{positions[0], positions[0]};
    for (const auto& pos : positions)
    {
      box.expand(pos);
    }
    return box;
  }
};

} // namespace obzcene
