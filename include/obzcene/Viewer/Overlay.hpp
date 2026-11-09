#pragma once

/**
 * @file Overlay.hpp
 * @brief Bitmap-font HUD painted onto the framebuffer.
 *
 * A tiny fixed 5×7 pixel font (ASCII 0x20..0x7e) used to draw the viewer's
 * on-screen stats panel and help line. Everything is rendered directly into
 * the FrameBuffer like any other geometry, so the overlay is pure CPU code in
 * src/Viewer/Overlay.cpp.
 *
 * @see src/Viewer/Overlay.cpp (font table + draw_stats layout),
 *      Viewer/Viewer.hpp (calls draw_stats each frame),
 *      Renderer/FrameBuffer.hpp (target)
 */

#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Color.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/Mesh.hpp"
#include "obzcene/Renderer/RenderStats.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <string>
#include <vector>

namespace obzcene::overlay
{

/**
 * @brief Width of a text run in pixels.
 * @param text  String to measure.
 * @return `size * kGlyphAdvance` (6 px per glyph).
 */
[[nodiscard]] auto text_width(const std::string& text) -> int;

/**
 * @brief Rasterize a text run at fixed position (top-left origin).
 * @param fb     Target framebuffer; pixels out of range are skipped.
 * @param x,y    Top-left corner of the first glyph.
 * @param text   Characters to draw (non-printable are skipped, advancing).
 * @param color  Colour stamped for every set bit.
 */
auto draw_text(FrameBuffer& fb, int x, int y, const std::string& text, const Color& color) -> void;

/**
 * @brief Draw the complete viewer HUD (stats, mesh info, lights, help).
 * @param fb            Target framebuffer.
 * @param stats         This frame's rasterizer counters.
 * @param mesh          Loaded geometry (triangle/vertex/bounds info).
 * @param textures      Diffuse maps (listed with each material's map name).
 * @param lights        Resolved light list (counted by type; shows the
 *                      `active_light` entry).
 * @param camera        Used to show position and bounds.
 * @param fps,frame_ms  Smoothed timing values.
 * @param wireframe     Renders "mode: wireframe/solid".
 * @param active_light  Index of the light the `[ ]`/arrow keys move.
 * @param headlamp      Whether the camera headlamp is enabled.
 */
auto draw_stats(FrameBuffer& fb, const RenderStats& stats, const Mesh& mesh,
                const std::vector<Texture>& textures, const std::vector<LightF>& lights,
                const CameraF& camera, float fps, float frame_ms, bool wireframe, usz active_light,
                bool headlamp) -> void;

} // namespace obzcene::overlay