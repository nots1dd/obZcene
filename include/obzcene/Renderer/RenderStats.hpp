#pragma once

/**
 * @file RenderStats.hpp
 * @brief Per-frame counters produced by the rasterizer and displayed by the
 *        overlay.
 *
 * Every field is incremented by the header-inline rasterizer at its respective
 * pipeline stage; obzcene::overlay::draw_stats paints them into the viewer
 * HUD. The culled/by-flag splits let you see *why* triangles were dropped
 * (backface, off-screen, out of clip range, or degenerate).
 *
 * @see Renderer/Rasterizer.hpp, Viewer/Overlay.hpp
 */

#include <cstddef>

namespace obzcene
{

/**
 * @brief Triangle/pixel accounting for one rendered frame.
 */
struct RenderStats
{
  /** @brief Triangles entering the rasterizer before any culling. */
  std::size_t triangles_processed{0};
  /** @brief Dropped by backface culling (winding). */
  std::size_t triangles_culled_backface{0};
  /** @brief Dropped because the bounding box misses the framebuffer. */
  std::size_t triangles_culled_viewport{0};
  /** @brief Dropped because every vertex is behind the near plane or beyond far. */
  std::size_t triangles_culled_nearfar{0};
  /** @brief Dropped for having zero area. */
  std::size_t triangles_degenerate{0};
  /** @brief Survived culling; rasterization was attempted. */
  std::size_t triangles_rendered{0};
  /** @brief Fragment shader invocations (pixels entering the depth/colour stage). */
  std::size_t pixels_shaded{0};
  /** @brief Pixels that passed the depth test and were written. */
  std::size_t pixels_written{0};

  /**
   * @brief Triangles that reached the raster stage.
   * @note Equivalent to `processed` minus all four cull/degenerate counters.
   */
  [[nodiscard]] auto visible_triangles() const -> std::size_t
  {
    return triangles_processed - triangles_culled_backface - triangles_culled_viewport -
           triangles_culled_nearfar - triangles_degenerate;
  }
};

} // namespace obzcene