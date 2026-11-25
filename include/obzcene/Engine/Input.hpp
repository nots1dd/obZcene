#pragma once

/**
 * @file Input.hpp
 * @brief Polled keyboard/mouse input state with edge detection
 *        (namespace obzcene::engine).
 *
 * The viewer's ad-hoc scancode polling grew organically and only worked
 * because one hardcoded loop owned it; games need a reusable, testable input
 * layer instead. obzcene::Input is a frame-sampled snapshot: the host calls
 * #begin_frame() after pumping events, then game code queries
 * down/pressed/released for the current frame. No SDL types leak into game
 * code; the host translates.
 *
 * @see Engine/Engine.hpp (drives it each frame), src/Viewer/Viewer.cpp
 */

#include <cstdint>
#include <utility>

#include "obzcene/Core/Types.hpp"

namespace obzcene
{

/** @brief Mouse buttons understood by the input layer. */
enum class MouseButton : std::uint8_t
{
  Left,
  Middle,
  Right,
  Count,
};

/** @brief Key identifiers understood by the input layer. */
enum class Key : std::uint8_t
{
  W,
  A,
  S,
  D,
  Q,
  E,
  R,
  G,
  T,
  F,
  O,
  L,
  Up,
  Down,
  Left,
  Right,
  PageUp,
  PageDown,
  Home,
  End,
  Space,
  Enter,
  Escape,
  Tab,
  Backspace,
  Delete,
  LeftBracket,
  RightBracket,
  Minus,
  Equals,
  F1,
  F2,
  F3,
  F4,
  F5,
  F9,
  Count,
};

/**
 * @brief Per-frame input snapshot with edge detection.
 *
 * `down` = held this frame; `pressed` = went down this frame;
 * `released` = went up this frame. Edge states last exactly one frame — call
 * #begin_frame() once per frame before querying.
 */
class Input
{
public:
  /**
   * @brief Sample the raw key state for a new frame.
   * @param state  Flat array indexed by `Key`; nonzero = held.
   */
  auto begin_frame(const std::uint8_t* state) -> void
  {
    for (usz i = 0; i < static_cast<usz>(Key::Count); ++i)
    {
      m_pressed[i]  = state[i] != 0 && !m_down[i];
      m_released[i] = state[i] == 0 && m_down[i];
      m_down[i]     = state[i] != 0;
    }
  }

  /** @brief True while the key is held. */
  [[nodiscard]] auto down(Key key) const -> bool { return m_down[static_cast<usz>(key)]; }
  /** @brief True only on the frame the key went down. */
  [[nodiscard]] auto pressed(Key key) const -> bool { return m_pressed[static_cast<usz>(key)]; }
  /** @brief True only on the frame the key went up. */
  [[nodiscard]] auto released(Key key) const -> bool { return m_released[static_cast<usz>(key)]; }

  /** @brief Relative mouse motion accumulated for the frame. */
  [[nodiscard]] auto mouse_delta() const -> std::pair<float, float>
  {
    return {m_mouse_dx, m_mouse_dy};
  }

  /**
   * @brief Sample the raw mouse-button state for a new frame.
   * @param state  Flat array indexed by `MouseButton`; nonzero = held.
   * @note Call together with the key sampling in #begin_frame.
   */
  auto begin_frame_buttons(const std::uint8_t* state) -> void
  {
    for (usz i = 0; i < static_cast<usz>(MouseButton::Count); ++i)
    {
      m_btn_pressed[i]  = state[i] != 0 && !m_btn_down[i];
      m_btn_released[i] = state[i] == 0 && m_btn_down[i];
      m_btn_down[i]     = state[i] != 0;
    }
  }

  /** @brief True while the button is held. */
  [[nodiscard]] auto down(MouseButton button) const -> bool
  {
    return m_btn_down[static_cast<usz>(button)];
  }
  /** @brief True only on the frame the button went down. */
  [[nodiscard]] auto pressed(MouseButton button) const -> bool
  {
    return m_btn_pressed[static_cast<usz>(button)];
  }
  /** @brief True only on the frame the button went up. */
  [[nodiscard]] auto released(MouseButton button) const -> bool
  {
    return m_btn_released[static_cast<usz>(button)];
  }

  /** @brief Record mouse motion (called by the host's event pump). */
  auto add_mouse_delta(float dx, float dy) -> void
  {
    m_mouse_dx += dx;
    m_mouse_dy += dy;
  }

  /** @brief Clear the accumulated mouse delta (start of each frame). */
  auto reset_mouse_delta() -> void
  {
    m_mouse_dx = 0.0F;
    m_mouse_dy = 0.0F;
  }

private:
  bool  m_down[static_cast<usz>(Key::Count)]{};
  bool  m_pressed[static_cast<usz>(Key::Count)]{};
  bool  m_released[static_cast<usz>(Key::Count)]{};
  bool  m_btn_down[static_cast<usz>(MouseButton::Count)]{};
  bool  m_btn_pressed[static_cast<usz>(MouseButton::Count)]{};
  bool  m_btn_released[static_cast<usz>(MouseButton::Count)]{};
  float m_mouse_dx{0.0F};
  float m_mouse_dy{0.0F};
};

} // namespace obzcene
