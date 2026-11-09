#pragma once

/**
 * @file MtlParser.hpp
 * @brief Header-only Wavefront MTL parser producing obzcene::Material.
 *
 * Reads the materials referenced by an OBJ's `mtllib` line; the resulting
 * vector is stored into ObjData::materials (via parse_obj), copied into
 * Mesh::materials by build_mesh, and indexed by both the rasterizer (shading)
 * and the texture loader (diffuse maps). Parsing is line-based and forgiving:
 * unknown keywords are skipped with a warning.
 *
 * @see File/Obj/Parser/ObjParser.hpp (caller),
 *      Core/Types.hpp (Material),
 *      Renderer/Shader.hpp (renderer/light consumer)
 */

#include "obzcene/Core/Logger.hpp"
#include "obzcene/Core/Types.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace obzcene
{

/**
 * @brief Parse a Wavefront `.mtl` file into a material list.
 * @param filepath  Path to the MTL file.
 * @return One Material per `newmtl` block, in file order. An empty vector on
 *         failure to open the file.
 *
 * Supported keywords: `newmtl`, `Ka`, `Kd`, `Ks`, `Ke`, `Tf`, `Ns`, `Ni`, `d`,
 * `Tr` (stored as `1 - Tr`), `illum`, `map_Kd`, `map_Ks`, `map_Ka` and
 * `map_bump`/`bump`/`map_Kn`. Lines before the first `newmtl` are warned
 * about and skipped.
 */
inline auto parse_mtl(const std::string& filepath) -> std::vector<Material>
{
  LOG_MODULE_INFO("MTL", "Parsing MTL file: {}", filepath);

  std::ifstream file(filepath);
  if (!file.is_open())
  {
    LOG_MODULE_ERROR("MTL", "Failed to open MTL file: {}", filepath);
    return {};
  }

  std::vector<Material> materials;
  Material*             current = nullptr;
  std::string           line;
  int                   line_number = 0;

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

    if (token == "newmtl")
    {
      materials.emplace_back();
      current = &materials.back();
      iss >> current->name;
      LOG_MODULE_TRACE("MTL", "  newmtl: '{}' at line {}", current->name, line_number);
    }
    else if (!current)
    {
      LOG_MODULE_WARN("MTL", "Skipping keyword '{}' outside material at line {}", token,
                      line_number);
      continue;
    }
    else if (token == "Ka")
    {
      iss >> current->ambient.x >> current->ambient.y >> current->ambient.z;
    }
    else if (token == "Kd")
    {
      iss >> current->diffuse.x >> current->diffuse.y >> current->diffuse.z;
    }
    else if (token == "Ks")
    {
      iss >> current->specular.x >> current->specular.y >> current->specular.z;
    }
    else if (token == "Ke")
    {
      iss >> current->emissive.x >> current->emissive.y >> current->emissive.z;
    }
    else if (token == "Tf")
    {
      iss >> current->transmission.x >> current->transmission.y >> current->transmission.z;
    }
    else if (token == "Ns")
    {
      iss >> current->shininess;
    }
    else if (token == "Ni")
    {
      iss >> current->ior;
    }
    else if (token == "d")
    {
      iss >> current->opacity;
    }
    else if (token == "Tr")
    {
      float tr = 0.0F;
      iss >> tr;
      current->opacity = 1.0F - tr;
    }
    else if (token == "illum")
    {
      iss >> current->illum;
    }
    else if (token == "map_Kd")
    {
      iss >> current->diffuse_texture;
    }
    else if (token == "map_Ks")
    {
      iss >> current->specular_texture;
    }
    else if (token == "map_Ka")
    {
      iss >> current->ambient_texture;
    }
    else if (token == "map_bump" || token == "bump" || token == "map_Kn")
    {
      iss >> current->normal_texture;
    }
    else
    {
      LOG_MODULE_WARN("MTL", "Unknown keyword '{}' at line {}", token, line_number);
    }
  }

  LOG_MODULE_INFO("MTL", "Parsed {} material(s) from '{}'", materials.size(), filepath);
  return materials;
}

} // namespace obzcene
