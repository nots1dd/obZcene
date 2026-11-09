#pragma once

/**
 * @file Shader.hpp
 * @brief Per-fragment shading: texture lookup, lighting, and quantization.
 *
 * This is the software-renderer equivalent of a fragment shader. It combines
 * the texture pipeline (obzcene::Texture + sample), the light model
 * (obzcene::Light::shade_all), material attributes from an
 * obzcene::Material and per-pixel interpolants from the rasterizer. Output is
 * an 8-bit obzcene::Color written into the FrameBuffer.
 *
 * @see Renderer/Rasterizer.hpp (calls shade_fragment per pixel),
 *      Renderer/Light.hpp, Texture/Texture.hpp
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Math/Vec4.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Color.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <vector>

namespace obzcene
{

/**
 * @brief Everything a fragment shader needs, bound from the frame pipeline.
 *
 * The camera drives view_dir; `lights` are the already-resolved WorldSpace
 * lights (obzcene::resolve_lights); `diffuse_textures[b]` matches
 * `materials[b]` by index (see src/Texture/TextureLoader.cpp).
 */
struct FragmentUniforms
{
  /** @brief Camera used to compute the view direction. */
  const CameraF& camera;
  /** @brief Resolved world-space lights to apply. */
  const std::vector<LightF>& lights;
  /** @brief Diffuse maps, indexed like `materials` (empty entry = no map). */
  const std::vector<Texture>& diffuse_textures;
  /** @brief Global scaling of each material's ambient term. */
  float ambient_intensity{0.1F};
};

/**
 * @brief Per-pixel interpolated attributes produced by the rasterizer.
 */
struct FragmentInput
{
  /** @brief World-space position (for distance lighting/view vector). */
  math::Vec3f world_position;
  /** @brief Interpolated normal (normalized in the shader). */
  math::Vec3f normal;
  /** @brief Interpolated texture coordinates (v flipped at sample time). */
  math::Vec2f uv;
  /** @brief Material (and texture) index for this triangle. */
  ui32 material_index{0};
};

/**
 * @brief Transform a world point to clip space via the camera.
 * @see obzcene::Camera::view_projection, used by the rasterizer vertex stage.
 */
[[nodiscard]] inline auto world_to_clip(const CameraF& camera, const math::Vec3f& world)
  -> math::Vec4f
{
  return camera.view_projection() * math::Vec4f{world.x, world.y, world.z, 1.0F};
}

/**
 * @brief Evaluate lighting + texturing for one fragment.
 *
 * The diffuse base colour is the material's `diffuse` (optionally modulated by
 * the diffuse map sampled with Linear filtering and Repeat wrap, v flipped to
 * match binary image rows). Lighting then accumulates `shade_all` plus the
 * material's emissive, and the result is clamped/quantized by
 * Color::from_normalized.
 *
 * @param input      Interpolated fragment attributes.
 * @param material   Surface material (see Core/Types.hpp).
 * @param uniforms   Frame-wide lighting/camera/texture state.
 */
[[nodiscard]] inline auto shade_fragment(const FragmentInput& input, const Material& material,
                                         const FragmentUniforms& uniforms) -> Color
{
  auto base = material.diffuse;
  if (input.material_index < uniforms.diffuse_textures.size())
  {
    const auto& tex = uniforms.diffuse_textures[input.material_index];
    if (!tex.empty())
    {
      const auto sampled =
        sample(tex, input.uv.x, 1.0F - input.uv.y, TextureFilter::Linear, TextureWrap::Repeat);
      base = math::Vec3f{sampled.x * material.diffuse.x, sampled.y * material.diffuse.y,
                         sampled.z * material.diffuse.z};
    }
  }
  const auto has_specular = material.specular != math::Vec3f{};
  const auto view_dir     = has_specular
                              ? math::normalized(uniforms.camera.position() - input.world_position)
                              : math::Vec3f{};
  auto shaded = shade_all(input.world_position, math::normalized(input.normal), view_dir, base,
                          material.specular, material.shininess,
                          material.ambient * uniforms.ambient_intensity, uniforms.lights);
  shaded      = math::Vec3f{shaded.x + material.emissive.x, shaded.y + material.emissive.y,
                       shaded.z + material.emissive.z};
  return Color::from_normalized(shaded);
}

} // namespace obzcene