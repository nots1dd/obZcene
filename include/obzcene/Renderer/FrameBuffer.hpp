#pragma once

/**
 * @file FrameBuffer.hpp
 * @brief CPU-side colour + depth target the rasterizer writes into.
 *
 * Two flat arrays indexed as `y*width + x`: an RGBA colour buffer (Color) and
 * a float depth buffer initialized to +infinity. The header-inline rasterizer
 * (Renderer/Rasterizer.hpp) draws into it, then it is either presented via
 * obzcene::Window (interactive) or handed to the image writers for snapshots
 * (obzcene::ImageWriter).
 *
 * @see Renderer/Rasterizer.hpp, Renderer/Window.hpp,
 *      Texture/ImageWriter.hpp
 */

#include "obzcene/Core/Types.hpp"
#include "obzcene/Renderer/Color.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace obzcene
{

/**
 * @brief Colour/depth render target with row-major linear storage.
 *
 * Accessors are split by index (flat) or by (x,y): both refer to the same
 * buffer. The `at(x, y)` / `depth_at(x, y)` forms assume in-range coordinates;
 * call #inside first.
 */
class FrameBuffer
{
public:
  /** @brief Uninitialized zero-sized framebuffer; use #resize before use. */
  FrameBuffer() = default;

  /**
   * @brief Allocate a framebuffer.
   * @param width,height  Size in pixels; the buffers are `width*height` long.
   */
  FrameBuffer(int width, int height)
      : m_width(width), m_height(height),
        m_color(static_cast<usz>(width) * static_cast<usz>(height)),
        m_depth(static_cast<usz>(width) * static_cast<usz>(height))
  {
  }

  /** @brief Reallocate for a new size; colour fills white, depth fills +inf. */
  auto resize(int width, int height) -> void
  {
    m_width  = width;
    m_height = height;
    m_color.assign(static_cast<usz>(width) * static_cast<usz>(height), Color{});
    m_depth.assign(static_cast<usz>(width) * static_cast<usz>(height), kInfinityDepth);
  }

  /** @brief Width in pixels. */
  [[nodiscard]] auto width() const -> int { return m_width; }

  /** @brief Height in pixels. */
  [[nodiscard]] auto height() const -> int { return m_height; }

  /** @brief True when (x, y) is within [0,width)×[0,height). */
  [[nodiscard]] auto inside(int x, int y) const -> bool
  {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
  }

  /** @brief Fill every pixel of the colour buffer with `color`. */
  auto clear(const Color& color) -> void { std::fill(m_color.begin(), m_color.end(), color); }

  /** @brief Fill the depth buffer with `depth` (default +infinity = empty). */
  auto clear_depth(float depth = kInfinityDepth) -> void
  {
    std::fill(m_depth.begin(), m_depth.end(), depth);
  }

  /** @brief Mutable colour at (x, y); caller must have checked #inside. */
  auto at(int x, int y) -> Color&
  {
    return m_color[static_cast<usz>(y) * static_cast<usz>(m_width) + static_cast<usz>(x)];
  }

  /** @brief Const colour at (x, y); caller must have checked #inside. */
  [[nodiscard]] auto at(int x, int y) const -> Color
  {
    return m_color[static_cast<usz>(y) * static_cast<usz>(m_width) + static_cast<usz>(x)];
  }

  /** @brief Mutable depth at (x, y). */
  auto depth_at(int x, int y) -> float&
  {
    return m_depth[static_cast<usz>(y) * static_cast<usz>(m_width) + static_cast<usz>(x)];
  }

  /** @brief Const depth at (x, y). */
  [[nodiscard]] auto depth_at(int x, int y) const -> float
  {
    return m_depth[static_cast<usz>(y) * static_cast<usz>(m_width) + static_cast<usz>(x)];
  }

  /** @brief Mutable colour at flat index (faster path used by the rasterizer). */
  auto color_at(usz index) -> Color& { return m_color[index]; }

  /** @brief Const colour at flat index (faster path used by the rasterizer). */
  [[nodiscard]] auto color_at(usz index) const -> Color { return m_color[index]; }

  /** @brief Mutable depth at flat index (faster path used by the rasterizer). */
  auto depth_at(usz index) -> float& { return m_depth[index]; }

  /** @brief Const depth at flat index. */
  [[nodiscard]] auto depth_at(usz index) const -> float { return m_depth[index]; }

  /**
   * @brief Flat byte view of the colour buffer (the writers expect RGBA bytes).
   * @see Texture/ImageWriter.hpp, obzcene::app::render_snapshot.
   */
  [[nodiscard]] auto color_data() const -> const ui8*
  {
    return reinterpret_cast<const ui8*>(m_color.data());
  }

  /** @brief Depth value meaning "nothing drawn here" (no valid depth). */
  static constexpr float kInfinityDepth = std::numeric_limits<float>::infinity();

private:
  int                m_width{0};
  int                m_height{0};
  std::vector<Color> m_color;
  std::vector<float> m_depth;
};

} // namespace obzcene
