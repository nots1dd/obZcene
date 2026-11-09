#pragma once

/**
 * @file Texture.hpp
 * @brief CPU-side image storage, filtering and procedural generators.
 *
 * A texture is a plain byte buffer plus width/height/channels; shading samples
 * it with #sample (nearest or bilinear) using a wrapping policy. It backs both
 * diffuse maps loaded from disk (obzcene::load_image / load_material_textures)
 * and cheap procedural wires in the viewer (make_checkerboard, make_solid).
 *
 * @see Texture/ImageLoader.hpp, Texture/TextureLoader.hpp,
 *      Renderer/Shader.hpp (shade_fragment samples diffuse maps)
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Color.hpp"

#include <cmath>
#include <cstdlib>
#include <vector>

namespace obzcene
{

/** @brief Texture fetcher: nearest texel or bilinear interpolation. */
enum class TextureFilter
{
  Nearest,
  Linear,
};

/** @brief How sample coordinates outside [0,1) are wrapped. */
enum class TextureWrap
{
  /** @brief Tiling: existing decimals repeat every size texels. */
  Repeat,
  /** @brief Edge clamp: last texel repeats beyond the bounds. */
  Clamp,
  /** @brief Mirroring: reflection, giving seamless tiling without seams. */
  Mirror,
};

/**
 * @brief A 2D image: interleaved `channels`-component bytes, row-major.
 *
 * Values are stored 0..255; `texel_rgb` normalizes them. `channels` is 4 for
 * disk-loaded images, but generators may use 3 for compact PPM output.
 */
struct Texture
{
  /** @brief Width in pixels. */
  int width{0};
  /** @brief Height in pixels. */
  int height{0};
  /** @brief Bytes per pixel (1 = grayscale, 3 = RGB, 4 = RGBA). */
  int channels{4};
  /** @brief Tight interleaved pixel storage, `width*height*channels` bytes. */
  std::vector<ui8> data;

  /** @brief True when there is nothing to sample (missing/invalid image). */
  [[nodiscard]] auto empty() const -> bool { return width <= 0 || height <= 0 || data.empty(); }

  /** @brief Allocate storage for a size, zero-filling every channel. */
  auto resize(int w, int h, int channel_count = 4) -> void
  {
    width    = w;
    height   = h;
    channels = channel_count;
    data.assign(static_cast<usz>(w) * static_cast<usz>(h) * static_cast<usz>(channel_count), 0);
  }
};

/**
 * @brief Convert a potentially out-of-range texel index into a valid one.
 * @param x    Raw integer coordinate (may be negative or ≥ size).
 * @param size Texture dimension along that axis.
 * @param wrap Wrapping policy (see obzcene::TextureWrap).
 */
[[nodiscard]] inline auto wrap_coord(int x, int size, TextureWrap wrap) -> int
{
  if (size <= 0)
  {
    return 0;
  }
  if (wrap == TextureWrap::Repeat)
  {
    x %= size;
    return x < 0 ? x + size : x;
  }
  if (wrap == TextureWrap::Mirror)
  {
    const auto period = 2 * size;
    x %= period;
    if (x < 0)
    {
      x += period;
    }
    return x < size ? x : period - 1 - x;
  }
  return x < 0 ? 0 : (x >= size ? size - 1 : x);
}

/**
 * @brief Normalized RGB colour of one texel (0..1), ignoring alpha.
 * @note Grayscale textures (1 channel) expand to `(v, v, v)`.
 */
[[nodiscard]] inline auto texel_rgb(const Texture& tex, int x, int y) -> math::Vec3f
{
  if (tex.channels == 1)
  {
    const auto v =
      tex.data[static_cast<usz>(y) * static_cast<usz>(tex.width) + static_cast<usz>(x)] / 255.0f;
    return math::Vec3f{v, v, v};
  }
  const auto i = (static_cast<usz>(y) * static_cast<usz>(tex.width) + static_cast<usz>(x)) *
                 static_cast<usz>(tex.channels);
  return math::Vec3f{tex.data[i] / 255.0f, tex.data[i + 1] / 255.0f, tex.data[i + 2] / 255.0f};
}

/**
 * @brief Point-sample a texture.
 * @param u,v  Coordinates in [0,1); v=0 is the top row.
 * @param wrap Out-of-range policy.
 */
[[nodiscard]] inline auto sample_nearest(const Texture& tex, float u, float v, TextureWrap wrap)
  -> math::Vec3f
{
  if (tex.empty())
  {
    return math::Vec3f{0.0f};
  }
  const auto x = wrap_coord(static_cast<int>(u * tex.width), tex.width, wrap);
  const auto y = wrap_coord(static_cast<int>(v * tex.height), tex.height, wrap);
  return texel_rgb(tex, x, y);
}

/**
 * @brief Bilinearly filter a texture (fractional texel centre lookup).
 * @param u,v  Coordinates in [0,1); v=0 is the top row.
 * @param wrap Out-of-range policy applied to each of the 2×2 taps.
 */
[[nodiscard]] inline auto sample_linear(const Texture& tex, float u, float v, TextureWrap wrap)
  -> math::Vec3f
{
  if (tex.empty())
  {
    return math::Vec3f{0.0f};
  }
  const auto tx  = u * tex.width - 0.5f;
  const auto ty  = v * tex.height - 0.5f;
  const auto x0  = std::floor(tx);
  const auto y0  = std::floor(ty);
  const auto sx  = tx - x0;
  const auto sy  = ty - y0;
  const auto px0 = wrap_coord(static_cast<int>(x0), tex.width, wrap);
  const auto px1 = wrap_coord(static_cast<int>(x0) + 1, tex.width, wrap);
  const auto py0 = wrap_coord(static_cast<int>(y0), tex.height, wrap);
  const auto py1 = wrap_coord(static_cast<int>(y0) + 1, tex.height, wrap);
  const auto top =
    texel_rgb(tex, px0, py0) + (texel_rgb(tex, px1, py0) - texel_rgb(tex, px0, py0)) * sx;
  const auto bottom =
    texel_rgb(tex, px0, py1) + (texel_rgb(tex, px1, py1) - texel_rgb(tex, px0, py1)) * sx;
  return top + (bottom - top) * sy;
}

/**
 * @brief Sample a texture with a given filter.
 * @see obzcene::shade_fragment (v is flipped before calling to match rows).
 */
[[nodiscard]] inline auto sample(const Texture& tex, float u, float v, TextureFilter filter,
                                 TextureWrap wrap) -> math::Vec3f
{
  return filter == TextureFilter::Linear ? sample_linear(tex, u, v, wrap)
                                         : sample_nearest(tex, u, v, wrap);
}

/**
 * @brief Build a checkerboard texture (two alternating colours).
 * @param cell_size  Side length in pixels of one square.
 * @param a,b        The two square colours.
 * @param channels   3 (RGB) or 4 (RGBA); 4 keeps the provided alpha.
 */
[[nodiscard]] inline auto make_checkerboard(int width, int height, int cell_size, const Color& a,
                                            const Color& b, int channels = 3) -> Texture
{
  Texture tex;
  tex.resize(width, height, channels);
  for (auto y = 0; y < height; ++y)
  {
    for (auto x = 0; x < width; ++x)
    {
      const auto  cell_x = x / cell_size;
      const auto  cell_y = y / cell_size;
      const auto& c      = (cell_x + cell_y) % 2 == 0 ? a : b;
      const auto  i      = (static_cast<usz>(y) * static_cast<usz>(width) + static_cast<usz>(x)) *
                     static_cast<usz>(channels);
      tex.data[i]     = c.r;
      tex.data[i + 1] = c.g;
      tex.data[i + 2] = c.b;
      if (channels == 4)
      {
        tex.data[i + 3] = c.a;
      }
    }
  }
  return tex;
}

/**
 * @brief Build a solid-colour texture.
 * @param channels  3 (RGB) or 4 (RGBA); 4 keeps the provided alpha.
 */
[[nodiscard]] inline auto make_solid(int width, int height, const Color& c, int channels = 3)
  -> Texture
{
  Texture tex;
  tex.resize(width, height, channels);
  const auto pixel_count = static_cast<usz>(width) * static_cast<usz>(height);
  for (usz i = 0; i < pixel_count; ++i)
  {
    tex.data[i * static_cast<usz>(channels)]     = c.r;
    tex.data[i * static_cast<usz>(channels) + 1] = c.g;
    tex.data[i * static_cast<usz>(channels) + 2] = c.b;
    if (channels == 4)
    {
      tex.data[i * static_cast<usz>(channels) + 3] = c.a;
    }
  }
  return tex;
}

} // namespace obzcene