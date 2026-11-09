#pragma once

/**
 * @file Window.hpp
 * @brief SDL2 window/renderer facade used by the interactive viewer.
 *
 * Owns the SDL window and texture so the framing loop can blit a
 * obzcene::FrameBuffer to screen each frame. It also pumps the input state
 * that obzcene::Viewer consumes (key state, relative-mouse deltas).
 * Not used in snapshot mode (src/App/Scene.cpp renders headless and writes
 * images instead).
 *
 * @see src/Renderer/Window.cpp (SDL init/teardown),
 *      Viewer/Viewer.hpp (drives handle_events + present),
 *      Renderer/FrameBuffer.hpp (the presented source texture)
 */

#include "obzcene/Renderer/FrameBuffer.hpp"

#include <SDL.h>

#include <string>
#include <utility>

namespace obzcene
{

/**
 * @brief An SDL2 window with an OpenGL-less software presentation path.
 *
 * The constructor creates the SDL window/renderer; `present()` uploads the
 * current framebuffer into an SDL_TEXTUREACCESS_STREAMING texture and blits it.
 * The class is non-copyable and non-movable (owns native SDL resources).
 */
class Window
{
public:
  /**
   * @brief Create a visible SDL window.
   * @param title   Window caption.
   * @param width,height  Logical client size in pixels.
   */
  Window(const std::string& title, int width, int height);
  /** @brief Destroy the SDL resources (and shut SDL down when last owner). */
  ~Window();

  Window(const Window&)                    = delete;
  auto operator=(const Window&) -> Window& = delete;
  Window(Window&&)                         = delete;
  auto operator=(Window&&) -> Window&      = delete;

  /** @brief Pump SDL events and accumulate relative mouse movement. */
  auto handle_events() -> void;
  /**
   * @brief Upload `fb` to the window texture and present it.
   * @note The internal texture is recreated only on size changes
   *       (#ensure_framebuffer).
   */
  auto present(const FrameBuffer& fb) -> void;

  /** @brief True while the window has no close/quit request. */
  [[nodiscard]] auto is_open() const -> bool;
  /** @brief Close the window (as if the user asked to quit). */
  auto close() -> void;
  /** @brief Live state of a key by scancode (pressed during this frame). */
  [[nodiscard]] auto key(SDL_Scancode scancode) const -> bool;
  /**
   * @brief Relative mouse movement accumulated since the last call.
   * @note Only meaningful while #set_relative_mouse(true) has been requested.
   */
  [[nodiscard]] auto take_mouse_delta() -> std::pair<long, long>;
  /** @brief Client width in pixels. */
  [[nodiscard]] auto width() const -> int;
  /** @brief Client height in pixels. */
  [[nodiscard]] auto height() const -> int;

  /** @brief Enable/disable the relative mouse mode (locks and hides cursor). */
  auto set_relative_mouse(bool enabled) -> void;

private:
  /** @brief Lazily (re)create the streaming texture matching a framebuffer size. */
  auto ensure_framebuffer(int width, int height) -> SDL_Texture*;

  SDL_Window*   m_window{nullptr};
  SDL_Renderer* m_renderer{nullptr};
  SDL_Texture*  m_framebuffer{nullptr};
  int           m_framebuffer_width{0};
  int           m_framebuffer_height{0};
  long          m_mouse_dx{0};
  long          m_mouse_dy{0};
  bool          m_open{false};
};

} // namespace obzcene