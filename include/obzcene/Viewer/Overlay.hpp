#pragma once

/**
 * @file Overlay.hpp
 * @brief Bitmap-font HUD and editor panels painted onto the framebuffer.
 *
 * A tiny fixed 5×7 pixel font (ASCII 0x20..0x7e) plus the editor's on-screen
 * furniture: the stats/HUD block, a status bar (mode/selection/messages), and
 * the file-browser panel. Everything is rendered directly into the
 * FrameBuffer like any other geometry, so the overlay is pure CPU code in
 * src/Viewer/Overlay.cpp.
 *
 * @see src/Viewer/Overlay.cpp (font table + draw implementations),
 *      Viewer/Viewer.hpp (drives these each frame),
 *      Viewer/FileBrowser.hpp (state rendered by draw_browser)
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

namespace obzcene
{
class FileBrowser;
class Scene;
} // namespace obzcene

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
 * @brief Everything the editor HUD needs to render one frame.
 */
struct HudInput
{
  /** @brief The scene being edited (objects/lights are summarized). */
  const Scene* scene;
  /** @brief This frame's rasterizer counters. */
  const RenderStats* stats;
  /** @brief Resolved light list (counted by type; shows the active entry). */
  const std::vector<LightF>* lights;
  /** @brief Camera used to show position. */
  const CameraF* camera;
  /** @brief Smoothed frames-per-second and frame time. */
  float fps{0.0F};
  float frame_ms{0.0F};
  /** @brief Renders "mode: wireframe/solid". */
  bool wireframe{false};
  /** @brief Index of the light the `[ ]`/arrow keys move. */
  usz active_light{0};
  /** @brief Whether the camera headlamp is enabled. */
  bool headlamp{false};
  /** @brief Index of the selected object, or -1 for none. */
  int selected_object{-1};
  /** @brief Editor mode label ("fly" / "grab" / "browse"). */
  std::string mode{"fly"};
  /** @brief Transient status message (empty = hidden). */
  std::string message;
};

/**
 * @brief Draw the complete editor HUD: stats block, scene summary, selection
 *        info and the key legend, plus a bottom status bar.
 */
auto draw_hud(FrameBuffer& fb, const HudInput& input) -> void;

/**
 * @brief Fill a rectangle with a colour (clipped to the framebuffer).
 *
 * Used for panel backgrounds in the editor UI (the browser and the status
 * bar); plain overwrite, no alpha blending.
 */
auto fill_rect(FrameBuffer& fb, int x, int y, int width, int height, const Color& color) -> void;

/**
 * @brief Draw the editor status bar (mode, selected object, message).
 * @param fb        Target framebuffer (drawn at the bottom row block).
 * @param mode      Current editor mode name ("fly", "grab", "browse", ...).
 * @param selection Label for the currently selected object (empty = none).
 * @param message   Transient status message (empty = hidden).
 */
auto draw_status_bar(FrameBuffer& fb, const std::string& mode, const std::string& selection,
                     const std::string& message) -> void;

/**
 * @brief Draw the file browser panel over the scene.
 * @param fb       Target framebuffer.
 * @param browser  The browser whose listing/cursor to render.
 *
 * Layout: centred panel, one row per entry, `>` marks the cursor, directories
 * shown with a trailing `/`. The listing scrolls so the cursor stays visible.
 */
auto draw_browser(FrameBuffer& fb, const FileBrowser& browser) -> void;

} // namespace obzcene::overlay
