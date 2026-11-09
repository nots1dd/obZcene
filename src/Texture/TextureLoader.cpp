/**
 * @file TextureLoader.cpp
 * @brief Implementation of load_material_textures.
 *
 * Produces the diffuse-texture table the fragment shader keys by material
 * index. Numeric indices here align with Mesh::materials as long as the mesh
 * was built from the same ObjData (see build_mesh), which is what the app
 * scene load path guarantees.
 *
 * @see Texture/TextureLoader.hpp (declaration), Texture/ImageLoader.hpp,
 *      Renderer/Shader.hpp (consumer)
 */

#include "obzcene/Texture/TextureLoader.hpp"

#include "obzcene/Core/Logger.hpp"
#include "obzcene/Texture/ImageLoader.hpp"

#include <string>

namespace obzcene
{

auto load_material_textures(const ObjData& data, const std::filesystem::path& model_dir)
  -> std::vector<Texture>
{
  auto textures = std::vector<Texture>(data.materials.size());
  for (std::size_t i = 0; i < data.materials.size(); ++i)
  {
    if (data.materials[i].diffuse_texture.empty())
    {
      continue;
    }
    const auto texture_path = model_dir / data.materials[i].diffuse_texture;
    textures[i]             = load_image(texture_path.string());
    if (textures[i].empty())
    {
      LOG_WARN("Failed to load texture '{}'", texture_path.string());
    }
    else
    {
      LOG_INFO("Texture '{}': {}x{}", texture_path.string(), textures[i].width, textures[i].height);
    }
  }
  return textures;
}

} // namespace obzcene