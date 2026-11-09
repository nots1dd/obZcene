#pragma once

/**
 * @file TextureLoader.hpp
 * @brief Build the per-frame diffuse texture table for a mesh.
 *
 * The returned table is index-aligned with `data.materials` (diffuse_textures
 * derived from each Material's `map_Kd`), which is exactly how the rasterizer
 * matches fragments to textures (shade_fragment indexes by material_index).
 *
 * @see src/Texture/TextureLoader.cpp (implementation),
 *      Renderer/Shader.hpp (FragmentUniforms::diffuse_textures),
 *      Texture/ImageLoader.hpp
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <filesystem>
#include <vector>

namespace obzcene
{

/**
 * @brief Load one diffuse texture per material from its `diffuse_texture`.
 * @param data       Parsed OBJ (materials + texture file names).
 * @param model_dir  Directory the model lives in; texture paths are resolved
 *                   relative to it.
 * @return A vector in the same order as `data.materials`; entries whose
 *         material has no map (or whose file fails to load) are empty
 *         textures, and a missing/invalid texture is logged and skipped.
 */
[[nodiscard]] auto load_material_textures(const ObjData&               data,
                                          const std::filesystem::path& model_dir)
  -> std::vector<Texture>;

} // namespace obzcene