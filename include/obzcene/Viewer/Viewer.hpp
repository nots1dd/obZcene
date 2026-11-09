#pragma once

/**
 * @file Viewer.hpp
 * @brief Interactive SDL window + frame loop + scene-editing controls.
 *
 * The viewer is the interactive front end: it owns the Window, the Camera,
 * the FrameBuffer and the editable Scene. Each frame it renders the scene
 * objects, draws the HUD/editor overlay (obzcene::overlay), and presents the
 * framebuffer. Input handling (fly camera, light/object manipulation, file
 * browser, scene save/load) lives in src/Viewer/Viewer.cpp.
 *
 * @see src/Viewer/Viewer.cpp (implementation),
 *      Renderer/Window.hpp, Viewer/Overlay.hpp, Viewer/Scene.hpp,
 *      Viewer/FileBrowser.hpp,
 *      App/App.hpp (run_viewer drives the camera/lights setup first)
 */

#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Viewer/FileBrowser.hpp"
#include "obzcene/Viewer/Scene.hpp"

#include <memory>
#include <string>
#include <vector>

namespace obzcene
{

class Window;

/** @brief The editor interaction mode. */
enum class ViewerMode
{
  /** @brief Free-fly camera (default). */
  Fly,
  /** @brief Moving the selected object (arrows/Home/End). */
  Grab,
  /** @brief The file browser modal is open. */
  Browse,
};

/**
 * @brief The interactive render loop and scene editor.
 *
 * Owns all per-frame state (window, camera, backbuffer, resolved lights,
 * selection). Call the `set_*` configuration hooks before `run`, or change
 * settings at runtime through the in-viewer keys (see src/Viewer/Viewer.cpp).
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
   * @brief Run the frame/editor loop until the window is closed.
   * @param scene       The scene to edit (objects added at runtime are kept).
   * @param scene_path  File the scene was loaded from / saves to by default.
   * @param no_textures Skip texture loading for objects added at runtime.
   */
  auto run(Scene scene, const std::string& scene_path, bool no_textures) -> void;

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
  /** @brief Handle one frame of input in Fly mode. */
  auto handle_fly_input(float dt) -> void;
  /** @brief Handle one frame of input in Grab mode (object manipulation). */
  auto handle_grab_input(float dt) -> void;
  /** @brief Handle one frame of input while the browser is open. */
  auto handle_browse_input() -> void;
  /** @brief Toggle wireframe/stats/headlamp style edge-triggered keys. */
  auto handle_toggles() -> void;
  /** @brief Move the active light (arrows/Home/End), if any. */
  auto handle_light_move(float move_speed) -> void;
  /** @brief Add the browser-picked file to the scene and start grabbing it. */
  auto add_object_from_browser(const std::filesystem::path& path) -> void;
  /** @brief Show a transient status-bar message. */
  auto set_message(std::string message) -> void;
  /** @brief Cycle the selected object (`selected + step`, wrapping). */
  auto cycle_selection(int step) -> void;
  /** @brief Save the scene (F5) to #m_scene_path. */
  auto save() -> void;
  /** @brief Reload the scene (F9) from #m_scene_path. */
  auto reload() -> void;

  std::unique_ptr<Window> m_window;
  CameraF                 m_camera;
  FrameBuffer             m_framebuffer;
  Scene                   m_scene;
  std::vector<LightF>     m_lights;
  /** @brief Per-frame resolved lights; reused across frames (no allocation churn). */
  std::vector<LightF> m_resolved_lights;
  FileBrowser         m_browser;
  std::string         m_scene_path;
  std::string         m_message;
  float               m_message_ttl{0.0F};
  math::Vec3f         m_reset_eye;
  math::Vec3f         m_reset_target;
  ViewerMode          m_mode{ViewerMode::Fly};
  int                 m_selected{-1};
  float               m_move_speed{1.0F};
  float               m_frame_ms{0.0F};
  float               m_ambient_intensity{0.1F};
  usz                 m_active_light{0};
  bool                m_wireframe{false};
  bool                m_show_stats{true};
  bool                m_headlamp{false};
  bool                m_no_textures{false};

  // Previous-frame key states for edge-triggered keys.
  bool m_prev_wireframe{false};
  bool m_prev_stats{false};
  bool m_prev_headlamp{false};
  bool m_prev_browser{false};
  bool m_prev_save{false};
  bool m_prev_load{false};
  bool m_prev_tab{false};
  bool m_prev_grab{false};
  bool m_prev_delete{false};
  bool m_prev_bracket{false};
  bool m_prev_browse_up{false};
  bool m_prev_browse_down{false};
  bool m_prev_browse_enter{false};
  bool m_prev_browse_back{false};
};

} // namespace obzcene
