#pragma once

/**
 * @file Draw2D.hpp
 * @brief Immediate-mode 2D drawing into a FrameBuffer: pixels, rectangles,
 *        squares, discs and lines — the building blocks for crosshairs, sky
 *        sprites and HUDs (namespace obzcene::draw2d).
 *
 * Games draw through Engine::framebuffer() inside Game::on_render (see
 * Engine/Engine.hpp). In the GPU backend the same framebuffer is composited
 * over the 3D frame as an alpha layer, so overlay code is backend-agnostic;
 * colours with alpha < 255 blend when `blend` is requested.
 *
 * All primitives clip against the framebuffer and are safe with off-screen
 * coordinates. `sky_only` variants paint nothing where a scene pixel was
 * already rendered (depth is no longer infinite) — that is what embeds sky
 * sprites behind terrain.
 *
 * @see Renderer/FrameBuffer.hpp, examples/sandbox.cpp (consumers)
 */

#include "obzcene/Renderer/Color.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"

#include <algorithm>

namespace obzcene::draw2d
{

/** @brief Write one pixel if (x, y) is inside the framebuffer. */
inline auto put(FrameBuffer& fb, int x, int y, const Color& color) -> void
{
  if (fb.inside(x, y))
  {
    fb.at(x, y) = color;
  }
}

/** @brief Alpha-blend (src-over) one pixel if (x, y) is inside the framebuffer. */
inline auto put_blended(FrameBuffer& fb, int x, int y, const Color& color) -> void
{
  if (!fb.inside(x, y))
  {
    return;
  }
  auto&      dst  = fb.at(x, y);
  const auto srca = static_cast<float>(color.a) / 255.0F;
  const auto mixc = [srca](ui8 s, ui8 d)
  {
    return static_cast<ui8>(static_cast<float>(s) * srca + static_cast<float>(d) * (1.0F - srca));
  };
  const auto out_a = static_cast<int>(color.a) + (255 - static_cast<int>(color.a)) * dst.a / 255;
  dst              = Color{mixc(color.r, dst.r), mixc(color.g, dst.g), mixc(color.b, dst.b),
              static_cast<ui8>(std::min(out_a, 255))};
}

/** @brief Shared per-pixel policy: bounds, then depth gating, then blending. */
inline auto paint(FrameBuffer& fb, int x, int y, const Color& color, bool sky_only, bool blend)
  -> void
{
  if (!fb.inside(x, y))
  {
    return;
  }
  if (sky_only && fb.depth_at(x, y) != FrameBuffer::kInfinityDepth)
  {
    return;
  }
  if (blend)
  {
    put_blended(fb, x, y, color);
  }
  else
  {
    put(fb, x, y, color);
  }
}

/**
 * @brief Filled rectangle, corners inclusive, clipped to the framebuffer.
 * @param sky_only  Paint only pixels with untouched (infinite) depth.
 * @param blend     Src-over blend instead of overwrite.
 */
inline auto rect(FrameBuffer& fb, int x0, int y0, int x1, int y1, const Color& color,
                 bool sky_only = false, bool blend = false) -> void
{
  if (x0 > x1 || y0 > y1)
  {
    return;
  }
  const auto x_begin = std::max(x0, 0);
  const auto x_end   = std::min(x1, fb.width() - 1);
  const auto y_begin = std::max(y0, 0);
  const auto y_end   = std::min(y1, fb.height() - 1);
  for (auto y = y_begin; y <= y_end; ++y)
  {
    for (auto x = x_begin; x <= x_end; ++x)
    {
      paint(fb, x, y, color, sky_only, blend);
    }
  }
}

/**
 * @brief Filled square centred on (cx, cy) — Minecraft-style sun/moon sprite.
 * @param half  Half the side length (total side = 2·half + 1).
 */
inline auto square(FrameBuffer& fb, int cx, int cy, int half, const Color& color,
                   bool sky_only = false, bool blend = false) -> void
{
  rect(fb, cx - half, cy - half, cx + half, cy + half, color, sky_only, blend);
}

/**
 * @brief Filled disc centred on (cx, cy).
 */
inline auto disc(FrameBuffer& fb, int cx, int cy, int radius, const Color& color,
                 bool sky_only = false, bool blend = false) -> void
{
  if (radius < 0)
  {
    return;
  }
  for (auto dy = -radius; dy <= radius; ++dy)
  {
    for (auto dx = -radius; dx <= radius; ++dx)
    {
      if (dx * dx + dy * dy <= radius * radius)
      {
        paint(fb, cx + dx, cy + dy, color, sky_only, blend);
      }
    }
  }
}

/**
 * @brief Straight line between two pixels (Bresenham), clipped per-pixel.
 */
inline auto line(FrameBuffer& fb, int x0, int y0, int x1, int y1, const Color& color,
                 bool sky_only = false, bool blend = false) -> void
{
  auto       dx  = std::abs(x1 - x0);
  auto       dy  = -std::abs(y1 - y0);
  const auto sx  = x0 < x1 ? 1 : -1;
  const auto sy  = y0 < y1 ? 1 : -1;
  auto       err = dx + dy;
  while (true)
  {
    paint(fb, x0, y0, color, sky_only, blend);
    if (x0 == x1 && y0 == y1)
    {
      break;
    }
    const auto e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

} // namespace obzcene::draw2d
