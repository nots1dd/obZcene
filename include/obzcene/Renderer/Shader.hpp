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
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"
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
/**
 * @brief Linear distance fog: fragments fade to `color` between `start` and
 *        `end` (world units from the camera). Disabled while end <= start.
 */
struct FogSettings
{
  /** @brief Colour distant fragments converge to (match the sky for depth cueing). */
  math::Vec3f color{0.5F, 0.66F, 1.0F};
  /** @brief Distance where fog starts to appear. */
  float start{0.0F};
  /** @brief Distance where fragments are fully fogged (0/negative = fog off). */
  float end{0.0F};

  /** @brief True when fog is active. */
  [[nodiscard]] auto enabled() const -> bool { return end > start; }
};

/**
 * @brief Fog blend factor for a fragment at `distance` from the camera.
 * @return 1 = unfogged, 0 = fully fogged (always 1 when fog is disabled).
 */
[[nodiscard]] inline auto fog_visibility(const FogSettings& fog, float distance) -> float
{
  if (!fog.enabled())
  {
    return 1.0F;
  }
  return math::clamp((fog.end - distance) / (fog.end - fog.start), 0.0F, 1.0F);
}

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
  /**
   * @brief Filtering for diffuse-map sampling. Linear (default) smooths,
   *        Nearest keeps pixel-art textures crisp.
   */
  TextureFilter filter{TextureFilter::Linear};
  /** @brief Distance fog (disabled by default; see FogSettings). */
  FogSettings fog{};
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
 * @brief Evaluate lighting + texturing for one fragment.
 *
 * The diffuse base colour is the material's `diffuse` (optionally modulated by
 * the diffuse map sampled with the frame's `filter` and Repeat wrap, v flipped
 * to match binary image rows). Lighting then accumulates `shade_all` plus the
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
        sample(tex, input.uv.x, 1.0F - input.uv.y, uniforms.filter, TextureWrap::Repeat);
      base = math::Vec3f{sampled.x * material.diffuse.x, sampled.y * material.diffuse.y,
                         sampled.z * material.diffuse.z};
    }
  }
  const auto has_specular = material.specular != math::Vec3f{};
  const auto view_dir     = has_specular
                              ? math::normalized(uniforms.camera.position() - input.world_position)
                              : math::Vec3f{};
  // Ambient is modulated by the diffuse base (Ka x base x intensity) — a flat
  // unmodulated ambient would wash every unlit face out to the same grey.
  const auto ambient = base * (material.ambient * uniforms.ambient_intensity);
  auto shaded = shade_all(input.world_position, math::normalized(input.normal), view_dir, base,
                          material.specular, material.shininess, ambient, uniforms.lights);
  shaded      = math::Vec3f{shaded.x + material.emissive.x, shaded.y + material.emissive.y,
                       shaded.z + material.emissive.z};
  // Distance fog last: it covers everything, including emissive.
  if (uniforms.fog.enabled())
  {
    const auto distance = math::length(uniforms.camera.position() - input.world_position);
    const auto keep     = fog_visibility(uniforms.fog, distance);
    shaded              = uniforms.fog.color * (1.0F - keep) + shaded * keep;
  }
  return Color::from_normalized(shaded);
}

} // namespace obzcene