#pragma once

/**
 * @file ObjParser.hpp
 * @brief Header-only Wavefront OBJ parser producing obzcene::ObjData.
 *
 * The front end of the geometry pipeline: turns a `.obj` file (plus its
 * `mtllib` MTL library) into vertex arrays, objects and faces. Faces keep
 * 0-based absolute indices into those arrays (`detail::resolve_index`), which
 * is exactly the layout obzcene::build_mesh expects. Grammar is line-oriented:
 * `v/vt/vn/o/g/usemtl/mtllib/f` plus a warning-only pass for unknown keywords.
 *
 * @see Renderer/Mesh.hpp (build_mesh consumes the output),
 *      Core/Types.hpp (ObjData/ObjFace/ObjVertexRef),
 *      File/Mtl/Parser/MtlParser.hpp (mtllib handling)
 */

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Logger.hpp"
#include "obzcene/Core/Types.hpp"
#include "obzcene/File/Mtl/Parser/MtlParser.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace obzcene
{

namespace detail
{

/**
 * @brief Split one `v/vt/vn` corner token of an `f` line into an ObjVertexRef.
 * @param token  e.g. `7/3/1`, `7/3`, `7//1` or a bare `7`.
 * @return The parsed reference, or `std::nullopt` when a field is present but
 *         not a valid integer.
 *
 * @note Index values returned here are still the raw file numbers (1-based or
 *       negative); see #resolve_index for normalization. Absent fields stay
 *       -1 (see ObjVertexRef).
 */
inline auto parse_face_vertex(const std::string& token) -> std::optional<ObjVertexRef>
{
  ObjVertexRef       ref;
  std::istringstream ss(token);
  std::string        part;

  const auto parse_field = [](const std::string& field, i32& out) -> bool
  {
    if (field.empty())
    {
      return true; // absent field, keep the -1 sentinel
    }
    const auto* first = field.data();
    const auto* last  = first + field.size();
    const auto  res   = std::from_chars(first, last, out);
    return res.ec == std::errc{} && res.ptr == last;
  };

  if (std::getline(ss, part, '/') && !parse_field(part, ref.position))
  {
    return std::nullopt;
  }
  if (std::getline(ss, part, '/') && !parse_field(part, ref.texcoord))
  {
    return std::nullopt;
  }
  if (std::getline(ss, part, '/') && !parse_field(part, ref.normal))
  {
    return std::nullopt;
  }
  return ref;
}

/**
 * @brief Convert a raw OBJ index to a 0-based absolute index.
 * @param index  File index: 1-based positive or negative-relative.
 * @param size   Element count of the target array (for negative indices).
 * @return 0-based index into the array; negative indices become
 *         `size + index`, positive become `index - 1`.
 */
inline auto resolve_index(i32 index, usz size) -> i32
{
  if (index < 0)
  {
    return narrow_cast<i32>(size) + index;
  }
  return index - 1;
}

/**
 * @brief Resolve a raw OBJ index in place and validate it against `size`.
 * @param index  Raw file index (1-based or negative-relative); never 0, which
 *               is not a valid OBJ index.
 * @return True when the resolved index lies in `[0, size)`.
 */
inline auto resolve_and_validate(i32& index, usz size) -> bool
{
  if (index == 0)
  {
    return false;
  }
  index = resolve_index(index, size);
  return index >= 0 && std::cmp_less(index, size);
}

} // namespace detail

/**
 * @brief Parse a Wavefront OBJ file (loading its MTL library as well).
 * @param filepath  Path to the `.obj` file.
 * @return ObjData with positions/texcoords/normals, objects, faces (indices
 *         resolved and normalized to 0-based by detail::resolve_index) and
 *         materials from `mtllib`. An empty ObjData on file-open failure.
 *
 * @note Faces with fewer than 3 corners are skipped (counted as degenerate
 *       and reported), as are faces with malformed or out-of-range vertex
 *       indices (reported separately); n-gons are kept as-is for
 *       triangulation in obzcene::build_mesh. `usemtl` names that do not match
 *       any parsed material fall back to index 0 with a warning.
 */
inline auto parse_obj(const std::string& filepath) -> ObjData
{
  LOG_MODULE_INFO("OBJ", "Parsing OBJ file: {}", filepath);

  std::ifstream file(filepath);
  if (!file.is_open())
  {
    LOG_MODULE_ERROR("OBJ", "Failed to open OBJ file: {}", filepath);
    return {};
  }

  ObjData     data;
  ObjObject*  current_object   = nullptr;
  ui32        current_material = 0;
  usz         thin_faces       = 0;
  usz         malformed_faces  = 0;
  usz         ngons            = 0;
  std::string line;
  int         line_number = 0;

  auto ensure_object = [&]()
  {
    if (!current_object)
    {
      data.objects.emplace_back();
      current_object       = &data.objects.back();
      current_object->name = "default";
    }
  };

  auto find_material = [&](const std::string& name) -> ui32
  {
    for (usz i = 0; i < data.materials.size(); ++i)
    {
      if (data.materials[i].name == name)
      {
        return narrow_cast<ui32>(i);
      }
    }
    LOG_MODULE_WARN("OBJ", "Material '{}' not found in loaded materials", name);
    return 0;
  };

  while (std::getline(file, line))
  {
    ++line_number;
    if (line.empty() || line[0] == '#')
    {
      continue;
    }

    std::istringstream iss(line);
    std::string        token;
    iss >> token;

    if (token == "v")
    {
      math::Vec3f pos;
      iss >> pos.x >> pos.y >> pos.z;
      data.positions.push_back(pos);
    }
    else if (token == "vt")
    {
      math::Vec2f tc;
      iss >> tc.x >> tc.y;
      data.texcoords.push_back(tc);
    }
    else if (token == "vn")
    {
      math::Vec3f norm;
      iss >> norm.x >> norm.y >> norm.z;
      data.normals.push_back(norm);
    }
    else if (token == "f")
    {
      ensure_object();

      ObjFace face;
      face.material_index = current_material;

      bool        face_valid = true;
      std::string vertex_token;
      while (iss >> vertex_token)
      {
        auto ref = detail::parse_face_vertex(vertex_token);
        if (!ref.has_value())
        {
          LOG_MODULE_WARN("OBJ", "Skipping malformed face vertex '{}' at line {}", vertex_token,
                          line_number);
          face_valid = false;
          break;
        }
        face.vertices.push_back(*ref);
      }

      // Resolve raw file indices (1-based or negative-relative) to 0-based
      // absolute indices. texcoord/normal keep -1 when absent; position is
      // mandatory, so an unresolvable one invalidates the whole face.
      for (auto& v : face.vertices)
      {
        if (!face_valid)
        {
          break;
        }
        if (!detail::resolve_and_validate(v.position, data.positions.size()))
        {
          face_valid = false;
          break;
        }
        if (v.texcoord != -1 && !detail::resolve_and_validate(v.texcoord, data.texcoords.size()))
        {
          v.texcoord = -1;
        }
        if (v.normal != -1 && !detail::resolve_and_validate(v.normal, data.normals.size()))
        {
          v.normal = -1;
        }
      }

      if (!face_valid)
      {
        ++malformed_faces;
        continue;
      }

      if (face.vertices.size() >= 3)
      {
        current_object->faces.push_back(face);
        if (face.vertices.size() > 3)
        {
          ++ngons;
        }
      }
      else
      {
        ++thin_faces;
      }
    }
    else if (token == "o")
    {
      data.objects.emplace_back();
      current_object = &data.objects.back();
      iss >> current_object->name;
      LOG_MODULE_TRACE("OBJ", "  object: '{}' at line {}", current_object->name, line_number);
    }
    else if (token == "g")
    {
      ensure_object();
      std::string name;
      iss >> name;
      if (!current_object->name.empty() && current_object->name != "default")
      {
        data.objects.emplace_back();
        current_object = &data.objects.back();
      }
      current_object->name = name;
      LOG_MODULE_TRACE("OBJ", "  group: '{}' at line {}", current_object->name, line_number);
    }
    else if (token == "usemtl")
    {
      ensure_object();
      std::string name;
      iss >> name;
      current_material = find_material(name);
      LOG_MODULE_TRACE("OBJ", "  usemtl: '{}' -> index {} at line {}", name, current_material,
                       line_number);
    }
    else if (token == "mtllib")
    {
      std::string mtl_name;
      iss >> mtl_name;
      data.material_lib = mtl_name;

      auto mtl_path = std::filesystem::path(filepath).parent_path() / mtl_name;
      LOG_MODULE_INFO("OBJ", "Loading material library: {}", mtl_path.string());

      data.materials = parse_mtl(mtl_path.string());
    }
    else if (token == "s")
    {
      // smooth shading — ignored for now, parsed for completeness
    }
    else
    {
      LOG_MODULE_WARN("OBJ", "Unknown keyword '{}' at line {}", token, line_number);
    }
  }

  if (ngons > 0)
  {
    LOG_MODULE_INFO("OBJ", "Triangulation pending: {} ngon face(s) in '{}'", ngons, filepath);
  }
  if (thin_faces > 0)
  {
    LOG_MODULE_WARN("OBJ", "{} degenerate face(s) skipped in '{}'", thin_faces, filepath);
  }
  if (malformed_faces > 0)
  {
    LOG_MODULE_WARN("OBJ", "{} malformed face(s) skipped in '{}'", malformed_faces, filepath);
  }

  LOG_MODULE_INFO("OBJ",
                  "Parsed '{}': {} positions, {} texcoords, {} normals, {} object(s), "
                  "{} material(s)",
                  filepath, data.positions.size(), data.texcoords.size(), data.normals.size(),
                  data.objects.size(), data.materials.size());

  return data;
}

} // namespace obzcene
