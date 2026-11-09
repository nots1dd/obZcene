#pragma once

/**
 * @file Viewer.hpp
 * @brief Interactive SDL window + frame loop + keyboard/mouse camera controls.
 *
 * The viewer is the interactive front end: it owns the Window, the Camera and
 * the FrameBuffer, and each frame it renders `run`'s mesh/lights/textures,
 * draws the stats overlay (obzcene::overlay), and presents the framebuffer.
 * Input handling (WASD/free-fly mouse look, wireframe/headlamp/light-cycle/UI
 * keys) lives in src/Viewer/Viewer.cpp.
 *
 * @see src/Viewer/Viewer.cpp (implementation),
 *      Renderer/Window.hpp, Viewer/Overlay.hpp,
 *      App/App.hpp (run_viewer drives the camera/lights setup first)
 */

#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/Mesh.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <memory>
#include <string>
#include <vector>

namespace obzcene
{

class Window;

/**
 * @brief The interactive render loop for one scene.
 *
 * Owns all per-frame state (window, camera, backbuffer, resolved lights).
 * Call the `set_*` configuration hooks before `run`, or change settings at
 * runtime through the in-viewer keys (see src/Viewer/Viewer.cpp).
 */
class Viewer
{
public:
  /**
   * @param title    Window caption.
   * @param width,height  Client/pixel size.
   */
  Viewer(std::string title, int width, int height);
  ~Viewer();

  Viewer(const Viewer&)                    = delete;
  auto operator=(const Viewer&) -> Viewer& = delete;

  /**
   * @brief Run the frame loop until the window is closed.
   * @param mesh      Geometry to draw each frame.
   * @param lights    Base lights (resolved per frame; copied so the Viewer can
   *                  cycle/headlamp-tweak without touching the caller's list).
   * @param textures  Diffuse maps indexed per material.
   */
  auto run(const Mesh& mesh, const std::vector<LightF>& lights,
           const std::vector<Texture>& textures) -> void;

  /** @brief Mutable access to the camera (used by App to apply the view). */
  [[nodiscard]] auto camera() -> CameraF&;
  /** @brief Const access to the camera. */
  [[nodiscard]] auto camera() const -> const CameraF&;
  /** @brief Set the per-second movement speed (see SceneView::speed). */
  auto set_move_speed(float speed) -> void;
  /** @brief Record the eye/target used by the reset key (R). */
  auto set_reset_pose(const math::Vec3f& eye, const math::Vec3f& target) -> void;
  /** @brief Toggle initial wireframe mode (also togglable with F1 at runtime). */
  auto set_wireframe(bool enabled) -> void;
  /** @brief Enable the camera-attached headlamp (F3 toggles at runtime). */
  auto set_headlamp(bool enabled) -> void;
  /** @brief Ambient intensity (scaled onto material ambient; see Shader). */
  auto set_ambient(float intensity) -> void;

private:
  std::unique_ptr<Window> m_window;
  CameraF                 m_camera;
  FrameBuffer             m_framebuffer;
  std::vector<LightF>     m_lights;
  /** @brief Per-frame resolved lights; reused across frames (no allocation churn). */
  std::vector<LightF> m_resolved_lights;
  math::Vec3f         m_reset_eye;
  math::Vec3f         m_reset_target;
  float               m_move_speed{1.0F};
  float               m_frame_ms{0.0F};
  float               m_ambient_intensity{0.1F};
  usz                 m_active_light{0};
  bool                m_wireframe{false};
  bool                m_show_stats{true};
  bool                m_headlamp{false};
};

} // namespace obzcene