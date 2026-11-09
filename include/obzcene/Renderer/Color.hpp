#pragma once

/**
 * @file Color.hpp
 * @brief 8-bit RGBA colour used by the render targets and PPM output.
 *
 * Colour values live as normalized Vec3 in the shading pipeline and are
 * quantized to byte components only at rasterization time by
 * #obzcene::Color::from_normalized (see the rasterizer in
 * Renderer/Rasterizer.hpp and obzcene::FrameBuffer). This matches how
 * PPM/BMP writers store pixels.
 *
 * @see Renderer/FrameBuffer.hpp, Renderer/Rasterizer.hpp,
 *      Texture/ImageWriter.hpp
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/Vec3.hpp"

namespace obzcene
{

/**
 * @brief An 8-bit RGBA pixel (defaults to opaque white).
 *
 * Components are stored in memory order r, g, b, a and are always in
 * 0..255. Construct from normalized floats via #from_normalized.
 */
struct Color
{
  /** @brief Red channel. */
  ui8 r{255};
  /** @brief Green channel. */
  ui8 g{255};
  /** @brief Blue channel. */
  ui8 b{255};
  /** @brief Alpha channel (unused by the renderer, kept for image formats). */
  ui8 a{255};

  /** @brief Default-constructed: opaque white. */
  constexpr Color() = default;

  /**
   * @param r_,g_,b_  Byte (0..255) components.
   * @param a_        Alpha byte, defaults to opaque.
   */
  constexpr Color(ui8 r_, ui8 g_, ui8 b_, ui8 a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}

  /**
   * @brief Quantize a normalized float colour to bytes (round to nearest).
   * @param rgb  Components expected in [0,1]; clamped to that range first.
   * @see Renderer/Shader.hpp (shade_fragment produces colours with this).
   */
  static auto from_normalized(const math::Vec3f& rgb) -> Color
  {
    const auto to_byte = [](float v) -> ui8
    {
      v = v < 0.0f ? 0.0f : v;
      v = v > 1.0f ? 1.0f : v;
      return static_cast<ui8>(v * 255.0f + 0.5f);
    };
    return Color{to_byte(rgb.x), to_byte(rgb.y), to_byte(rgb.z)};
  }
};

} // namespace obzcene