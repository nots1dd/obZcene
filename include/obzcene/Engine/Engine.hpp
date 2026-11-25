#pragma once

/**
 * @file Engine.hpp
 * @brief The game-facing engine facade: window, scene, camera, input, assets,
 *        audio — one object to drive a game (namespace obzcene).
 *
 * obzcene::Engine owns the runtime state and runs the frame loop;
 * obzcene::Game is the user hook point (init/update/render callbacks). A
 * minimal game is:
 *
 * @code
 *   class MyGame : public obzcene::Game
 *   {
 *     void on_update(obzcene::Engine& engine, float dt) override
 *     {
 *       if (engine.input().pressed(obzcene::Key::Space)) { ... }
 *     }
 *   };
 *   int main() { obzcene::Engine engine{{"My Game", 1280, 720}}; return engine.run(MyGame{}); }
 * @endcode
 *
 * The editor (src/App, src/Viewer) is built on the same primitives but is a
 * separate, heavier tool — games do not pay for it.
 *
 * @see src/Engine/Engine.cpp (implementation), Engine/Input.hpp,
 *      Engine/Assets.hpp, Engine/Audio.hpp
 */

#include "obzcene/Engine/Assets.hpp"
#include "obzcene/Engine/Audio.hpp"
#include "obzcene/Engine/Input.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/GpuRenderer.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/RenderStats.hpp"
#include "obzcene/Viewer/Scene.hpp"

#include <memory>
#include <string>

namespace obzcene
{

class Window;

/** @brief Startup configuration for an Engine. */
struct EngineConfig
{
  /** @brief Window caption. */
  std::string title{"obZcene game"};
  /** @brief Window/framebuffer size. */
  int width{1280};
  int height{720};
  /** @brief Vertical field of view in degrees. */
  float fov_y{60.0F};
  /** @brief Start in wireframe mode (F1 toggles at runtime). */
  bool wireframe{false};
  /** @brief Attach a directional headlamp to the camera. */
  bool headlamp{false};
  /** @brief Global ambient intensity. */
  float ambient{0.1F};
  /** @brief Skip texture loading (headless/CI runs). */
  bool no_textures{false};
  /** @brief Show the FPS/stats HUD (F2 toggles at runtime). */
  bool show_hud{true};
  /** @brief Enable the built-in fly camera (WASD + mouse). */
  bool fly_camera{true};
  /**
   * @brief Mouse-look sensitivity multiplier for the fly camera
   *        (1.0 = default speed; 0.5 = slower, 2.0 = faster).
   */
  float mouse_sensitivity{1.0F};
  /** @brief Background ("sky") colour the framebuffer clears to each frame. */
  Color clear_color{22, 22, 28};
  /** @brief Vertical sky gradient instead of a flat clear colour. */
  bool  sky_gradient{false};
  Color sky_top{52, 118, 214};     ///< zenith (gradient start)
  Color sky_bottom{120, 168, 255}; ///< horizon (gradient end)
  /**
   * @brief Diffuse texture filtering (Nearest keeps pixel-art textures crisp,
   *        Linear smooths them).
   */
  TextureFilter texture_filter{TextureFilter::Linear};
  /** @brief Root directory for asset lookups. */
  std::string assets_dir{"assets"};
  /**
   * @brief Renderer backend: Cpu (software rasterizer, default) or Gpu
   *        (OpenGL 3.3). The scene API is identical; in GPU mode the CPU
   *        framebuffer becomes a transparent overlay layer composited over
   *        the 3D frame, so Game::on_render/HUD code is backend-agnostic.
   */
  RendererBackend backend{RendererBackend::Cpu};
  /**
   * @brief Distance fog (disabled while fog.end <= fog.start). Games usually
   *        track fog.color to the sky colour for depth cueing.
   */
  FogSettings fog{};
  /** @brief Scene file to load at startup (empty = start with an empty scene). */
  std::string scene_file;
};

class Game;

/**
 * @brief The engine facade: one object owning the whole runtime.
 */
class Engine
{
public:
  explicit Engine(EngineConfig config = {});
  ~Engine();

  Engine(const Engine&)                    = delete;
  auto operator=(const Engine&) -> Engine& = delete;

  /** @brief The scene being rendered and mutated by the game. */
  [[nodiscard]] auto scene() -> Scene&;
  /** @brief The render camera (move it directly for scripted cameras). */
  [[nodiscard]] auto camera() -> CameraF&;
  /** @brief Per-frame input state (see Input). */
  [[nodiscard]] auto input() const -> const Input&;
  /** @brief The frame being rendered (draw custom overlays in Game::on_render). */
  [[nodiscard]] auto framebuffer() -> FrameBuffer&;
  /** @brief Named asset registry (models, audio). */
  [[nodiscard]] auto assets() -> Assets&;
  /** @brief Audio playback (offline-safe no-ops when no device exists). */
  [[nodiscard]] auto audio() -> Audio&;
  /** @brief Current frame delta time in seconds. */
  [[nodiscard]] auto dt() const -> float;
  /** @brief Seconds since run() started. */
  [[nodiscard]] auto time() const -> float;
  /** @brief Smoothed frames-per-second estimate. */
  [[nodiscard]] auto fps() const -> float;
  /** @brief True while the game loop should keep running. */
  [[nodiscard]] auto is_running() const -> bool;
  /** @brief Ask the loop to exit at the end of this frame. */
  auto quit() -> void;
  /** @brief Wireframe rendering (F1 toggles at runtime). */
  auto set_wireframe(bool enabled) -> void;

  /** @brief Background ("sky") colour the framebuffer clears to each frame. */
  auto set_clear_color(const Color& color) -> void;
  /**
   * @brief Enable/disable a vertical sky gradient background (`sky_top` at
   *        the zenith, `sky_bottom` at the horizon). Replaces the flat
   *        clear colour while enabled.
   */
  auto set_sky_gradient(bool enabled, const Color& top, const Color& bottom) -> void;
  /** @brief Current sky gradient colours (valid regardless of the enable flag). */
  [[nodiscard]] auto sky_top() const -> const Color&;
  /** @copydoc sky_top */
  [[nodiscard]] auto sky_bottom() const -> const Color&;
  /** @brief Diffuse texture filtering (Nearest = crisp pixel art, Linear = smooth). */
  auto set_texture_filter(TextureFilter filter) -> void;

  /** @brief Distance fog settings (set end <= start to disable). */
  auto set_fog(const FogSettings& fog) -> void;
  /** @brief Current distance fog settings. */
  [[nodiscard]] auto fog() const -> const FogSettings&;

  /**
   * @brief Mouse-look sensitivity for the fly camera; 1.0 is the default
   *        speed. Games can change it at runtime (e.g. an aim-down-sights key
   *        that halves it).
   */
  auto set_mouse_sensitivity(float sensitivity) -> void;
  /** @brief Current mouse-look sensitivity multiplier. */
  [[nodiscard]] auto mouse_sensitivity() const -> float;

  /** @brief The active renderer backend. */
  [[nodiscard]] auto backend() const -> RendererBackend;
  /** @brief GPU renderer (null unless EngineConfig::backend == Gpu). */
  [[nodiscard]] auto gpu() -> GpuRenderer*;

  /**
   * @brief Run the game loop until the window closes or quit() is called.
   * @param game  User callbacks; must outlive the call.
   * @return 0 on clean exit.
   */
  auto run(Game& game) -> int;

private:
  /** @brief Pump events into the input snapshot. */
  auto pump_input() -> void;

  EngineConfig                 m_config;
  std::unique_ptr<Window>      m_window;
  std::unique_ptr<GpuRenderer> m_gpu; // after m_window: destroyed while the GL context lives
  CameraF                      m_camera;
  FrameBuffer                  m_framebuffer;
  Scene                        m_scene;
  Input                        m_input;
  Assets                       m_assets;
  Audio                        m_audio;
  std::vector<LightF>          m_resolved_lights;
  RenderStats                  m_stats;
  float                        m_dt{0.0F};
  float                        m_time{0.0F};
  float                        m_frame_ms{0.0F};
  bool                         m_running{false};
  bool                         m_wireframe{false};
  bool                         m_show_hud{true};
  bool                         m_prev_wireframe_key{false};
  bool                         m_prev_hud_key{false};
};

/**
 * @brief User hook point for a game. All callbacks are optional.
 */
class Game
{
public:
  virtual ~Game() = default;

  /** @brief Called once before the first frame; load assets and build the scene. */
  virtual auto on_init(Engine& engine) -> void { (void)engine; }
  /** @brief Called once per frame with the frame's delta time. */
  virtual auto on_update(Engine& engine, float dt) -> void
  {
    (void)engine;
    (void)dt;
  }
  /**
   * @brief Called after the scene render, before present — draw HUD/custom
   *        overlays here via engine.framebuffer().
   */
  virtual auto on_render(Engine& engine) -> void { (void)engine; }
  /** @brief Called once after the loop exits. */
  virtual auto on_shutdown(Engine& engine) -> void { (void)engine; }
};

} // namespace obzcene
