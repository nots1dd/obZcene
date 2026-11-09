#pragma once

/**
 * @file ImageLoader.hpp
 * @brief Decode an image file into a CPU texture via SDL_image.
 *
 * The whole `include/obzcene/Texture/*` texture path (loading, texture table
 * assembly in TextureLoader.cpp, sampling in Shader.hpp) feeds the rasterizer;
 * this is the only place where the png/jpg/bmp/tga formats are decoded.
 *
 * @see Texture/TextureLoader.hpp (per-model diffuse table),
 *      Renderer/Shader.hpp (sampling)
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <string>

#include <SDL_image.h>

namespace obzcene
{

/**
 * @brief Load an image from disk and flatten it to RGBA.
 * @param path  File to decode (any format SDL_image supports).
 * @return An RGBA texture (channels = 4, pixels compacted, no pitch padding).
 *         On failure (missing file / unsupported format) the returned texture
 *         is empty and obzcene::Texture::empty() is true, so callers can treat
 *         it as "no texture".
 */
[[nodiscard]] inline auto load_image(const std::string& path) -> Texture
{
  Texture      tex;
  SDL_Surface* raw = IMG_Load(path.c_str());
  if (raw == nullptr)
  {
    return tex;
  }

  SDL_Surface* surface = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(raw);
  if (surface == nullptr)
  {
    return tex;
  }

  tex.width    = surface->w;
  tex.height   = surface->h;
  tex.channels = 4;
  tex.data.resize(static_cast<usz>(surface->w) * static_cast<usz>(surface->h) * 4);

  const auto* src = static_cast<const ui8*>(surface->pixels);
  for (auto y = 0; y < surface->h; ++y)
  {
    const auto src_row = static_cast<usz>(y) * surface->pitch;
    const auto dst_row = static_cast<usz>(y) * static_cast<usz>(surface->w) * 4;
    for (auto x = 0; x < surface->w; ++x)
    {
      const auto i    = dst_row + static_cast<usz>(x) * 4;
      const auto j    = src_row + static_cast<usz>(x) * 4;
      tex.data[i]     = src[j];
      tex.data[i + 1] = src[j + 1];
      tex.data[i + 2] = src[j + 2];
      tex.data[i + 3] = src[j + 3];
    }
  }

  SDL_FreeSurface(surface);
  return tex;
}

} // namespace obzcene