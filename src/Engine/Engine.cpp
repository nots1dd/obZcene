/**
 * @file Engine.cpp
 * @brief The engine facade implementation: window, loop, input plumbing.
 *
 * Engine::run drives the frame loop: pump input, run the game callbacks,
 * render the scene, present. The built-in fly camera (WASD + QE/PgUp/PgDn +
 * mouse look) can be switched off via EngineConfig::fly_camera for games that
 * drive the camera themselves. Input is exposed to games as the polled,
 * edge-detected obzcene::Input snapshot (Engine/Input.hpp).
 *
 * @see Engine/Engine.hpp (declarations)
 */

#include "obzcene/Engine/Engine.hpp"

#include "obzcene/Core/Logger.hpp"
#include "obzcene/Renderer/Rasterizer.hpp"
#include "obzcene/Renderer/Window.hpp"
#include "obzcene/Viewer/Overlay.hpp"
#include "obzcene/Viewer/SceneFile.hpp"

#include <SDL.h>

#include <array>
#include <utility>

namespace obzcene
{

namespace
{

/** @brief SDL scancode for an engine Key (single translation table). */
constexpr auto to_scancode(Key key) -> SDL_Scancode
{
  switch (key)
  {
    case Key::W:
      return SDL_SCANCODE_W;
    case Key::A:
      return SDL_SCANCODE_A;
    case Key::S:
      return SDL_SCANCODE_S;
    case Key::D:
      return SDL_SCANCODE_D;
    case Key::Q:
      return SDL_SCANCODE_Q;
    case Key::E:
      return SDL_SCANCODE_E;
    case Key::R:
      return SDL_SCANCODE_R;
    case Key::G:
      return SDL_SCANCODE_G;
    case Key::T:
      return SDL_SCANCODE_T;
    case Key::F:
      return SDL_SCANCODE_F;
    case Key::O:
      return SDL_SCANCODE_O;
    case Key::L:
      return SDL_SCANCODE_L;
    case Key::Up:
      return SDL_SCANCODE_UP;
    case Key::Down:
      return SDL_SCANCODE_DOWN;
    case Key::Left:
      return SDL_SCANCODE_LEFT;
    case Key::Right:
      return SDL_SCANCODE_RIGHT;
    case Key::PageUp:
      return SDL_SCANCODE_PAGEUP;
    case Key::PageDown:
      return SDL_SCANCODE_PAGEDOWN;
    case Key::Home:
      return SDL_SCANCODE_HOME;
    case Key::End:
      return SDL_SCANCODE_END;
    case Key::Space:
      return SDL_SCANCODE_SPACE;
    case Key::Enter:
      return SDL_SCANCODE_RETURN;
    case Key::Escape:
      return SDL_SCANCODE_ESCAPE;
    case Key::Tab:
      return SDL_SCANCODE_TAB;
    case Key::Backspace:
      return SDL_SCANCODE_BACKSPACE;
    case Key::Delete:
      return SDL_SCANCODE_DELETE;
    case Key::LeftBracket:
      return SDL_SCANCODE_LEFTBRACKET;
    case Key::RightBracket:
      return SDL_SCANCODE_RIGHTBRACKET;
    case Key::Minus:
      return SDL_SCANCODE_MINUS;
    case Key::Equals:
      return SDL_SCANCODE_EQUALS;
    case Key::F1:
      return SDL_SCANCODE_F1;
    case Key::F2:
      return SDL_SCANCODE_F2;
    case Key::F3:
      return SDL_SCANCODE_F3;
    case Key::F4:
      return SDL_SCANCODE_F4;
    case Key::F5:
      return SDL_SCANCODE_F5;
    case Key::F9:
      return SDL_SCANCODE_F9;
    case Key::Count:
      break;
  }
  return SDL_SCANCODE_UNKNOWN;
}

constexpr float kMouseSensitivity = 0.0025F;
constexpr float kMoveMultiplier   = 2.0F;

/** @brief Fly-camera speed for a scene: scaled to the scene's bounding radius. */
[[nodiscard]] auto scene_move_speed(const Scene& scene) -> float
{
  const auto bounds = scene.world_bounds();
  const auto radius = std::max(math::length(bounds.max - bounds.min) * 0.5F, 1.0e-3F);
  return std::max(radius * 0.25F, 0.05F);
}

} // namespace

Engine::Engine(EngineConfig config) : m_config(std::move(config)), m_assets(m_config.assets_dir)
{
  Logger::init();
  m_window = std::make_unique<Window>(m_config.title, m_config.width, m_config.height,
                                      m_config.backend == RendererBackend::Gpu);
  if (m_window->is_gpu())
  {
    m_gpu = std::make_unique<GpuRenderer>();
    if (!m_gpu->init())
    {
      LOG_ERROR("Engine: GPU backend init failed: {}", m_gpu->last_error());
      m_gpu.reset();
      m_window->close();
    }
    else
    {
      LOG_INFO("Engine: GPU backend (OpenGL 3.3 core) active");
    }
  }
  m_framebuffer = FrameBuffer{m_config.width, m_config.height};
  m_camera.set_perspective(math::deg_to_rad(m_config.fov_y),
                           static_cast<float>(m_config.width) / m_config.height, 0.05F, 2000.0F);
  m_wireframe = m_config.wireframe;
  m_show_hud  = m_config.show_hud;
  m_audio.init();
}

Engine::~Engine()
{
  m_audio.shutdown();
  Logger::shutdown();
}

auto Engine::scene() -> Scene& { return m_scene; }
auto Engine::camera() -> CameraF& { return m_camera; }
auto Engine::input() const -> const Input& { return m_input; }
auto Engine::framebuffer() -> FrameBuffer& { return m_framebuffer; }
auto Engine::assets() -> Assets& { return m_assets; }
auto Engine::audio() -> Audio& { return m_audio; }
auto Engine::dt() const -> float { return m_dt; }
auto Engine::time() const -> float { return m_time; }
auto Engine::fps() const -> float { return m_frame_ms > 0.0F ? 1000.0F / m_frame_ms : 0.0F; }
auto Engine::is_running() const -> bool { return m_running; }
auto Engine::quit() -> void { m_running = false; }
auto Engine::backend() const -> RendererBackend { return m_config.backend; }
auto Engine::gpu() -> GpuRenderer* { return m_gpu.get(); }
auto Engine::set_wireframe(bool enabled) -> void { m_wireframe = enabled; }

auto Engine::set_clear_color(const Color& color) -> void { m_config.clear_color = color; }

auto Engine::set_sky_gradient(bool enabled, const Color& top, const Color& bottom) -> void
{
  m_config.sky_gradient = enabled;
  m_config.sky_top      = top;
  m_config.sky_bottom   = bottom;
}

auto Engine::sky_top() const -> const Color& { return m_config.sky_top; }
auto Engine::sky_bottom() const -> const Color& { return m_config.sky_bottom; }

auto Engine::set_texture_filter(TextureFilter filter) -> void { m_config.texture_filter = filter; }

auto Engine::set_fog(const FogSettings& fog) -> void { m_config.fog = fog; }

auto Engine::fog() const -> const FogSettings& { return m_config.fog; }

auto Engine::set_mouse_sensitivity(float sensitivity) -> void
{
  m_config.mouse_sensitivity = sensitivity < 0.0F ? 0.0F : sensitivity;
}

auto Engine::mouse_sensitivity() const -> float { return m_config.mouse_sensitivity; }

auto Engine::pump_input() -> void
{
  const auto*                                            state = SDL_GetKeyboardState(nullptr);
  std::array<std::uint8_t, static_cast<usz>(Key::Count)> raw{};
  for (usz i = 0; i < raw.size(); ++i)
  {
    raw[i] = static_cast<std::uint8_t>(state[to_scancode(static_cast<Key>(i))] != 0);
  }
  m_input.begin_frame(raw.data());

  // Mouse buttons ride the same edge-detection path as keys.
  const auto buttons = SDL_GetMouseState(nullptr, nullptr);
  std::array<std::uint8_t, static_cast<usz>(MouseButton::Count)> raw_buttons{};
  raw_buttons[static_cast<usz>(MouseButton::Left)]   = (buttons & SDL_BUTTON_LMASK) != 0;
  raw_buttons[static_cast<usz>(MouseButton::Middle)] = (buttons & SDL_BUTTON_MMASK) != 0;
  raw_buttons[static_cast<usz>(MouseButton::Right)]  = (buttons & SDL_BUTTON_RMASK) != 0;
  m_input.begin_frame_buttons(raw_buttons.data());
}

auto Engine::run(Game& game) -> int
{
  if (!m_window->is_open())
  {
    LOG_ERROR("Engine: window could not be opened");
    return 1;
  }

  if (!m_config.scene_file.empty())
  {
    const auto data = load_scene_file(m_config.scene_file);
    if (data.has_value())
    {
      const auto dir = std::filesystem::path{m_config.scene_file}.parent_path();
      apply_scene_file(m_scene, *data, dir.empty() ? "." : dir, m_config.no_textures);
      if (data->camera_position.has_value())
      {
        m_camera.set_position(*data->camera_position);
        m_camera.set_orientation(data->camera_yaw.value_or(0.0F),
                                 data->camera_pitch.value_or(0.0F));
      }
    }
    else
    {
      LOG_WARN("Engine: scene file '{}' not loaded; starting empty", m_config.scene_file);
    }
  }

  m_window->set_relative_mouse(true);
  m_running = true; // before on_init so the game can quit() during init
  game.on_init(*this);
  auto last_frame = SDL_GetTicks64();
  while (m_running && m_window->is_open())
  {
    const auto now      = SDL_GetTicks64();
    m_dt                = static_cast<float>(now - last_frame) / 1000.0F;
    m_time              = static_cast<float>(now) / 1000.0F;
    last_frame          = now;
    const auto frame_ms = m_dt * 1000.0F;
    m_frame_ms          = m_frame_ms * 0.9F + frame_ms * 0.1F;

    m_window->handle_events();
    pump_input();

    // Built-in toggles.
    const auto wireframe_down = m_input.pressed(Key::F1);
    if (wireframe_down && !m_prev_wireframe_key)
    {
      m_wireframe = !m_wireframe;
    }
    m_prev_wireframe_key = wireframe_down;

    const auto hud_down = m_input.pressed(Key::F2);
    if (hud_down && !m_prev_hud_key)
    {
      m_show_hud = !m_show_hud;
    }
    m_prev_hud_key = hud_down;

    if (m_input.pressed(Key::Escape))
    {
      quit();
    }

    // The mouse delta rides the input snapshot regardless of camera mode, so
    // games with their own camera (fly_camera = false) can still read it.
    const auto mouse = m_window->take_mouse_delta();
    m_input.reset_mouse_delta();
    m_input.add_mouse_delta(static_cast<float>(mouse.first), static_cast<float>(mouse.second));

    if (m_config.fly_camera)
    {
      const auto sensitivity = kMouseSensitivity * m_config.mouse_sensitivity;
      m_camera.yaw_by(-static_cast<float>(mouse.first) * sensitivity);
      m_camera.pitch_by(-static_cast<float>(mouse.second) * sensitivity);

      const auto move_speed = m_dt * scene_move_speed(m_scene) * kMoveMultiplier;
      if (m_input.down(Key::W))
      {
        m_camera.move_forward(move_speed);
      }
      if (m_input.down(Key::S))
      {
        m_camera.move_backward(move_speed);
      }
      if (m_input.down(Key::A))
      {
        m_camera.move_left(move_speed);
      }
      if (m_input.down(Key::D))
      {
        m_camera.move_right(move_speed);
      }
      if (m_input.down(Key::E) || m_input.down(Key::PageUp))
      {
        m_camera.move_up(move_speed);
      }
      if (m_input.down(Key::Q) || m_input.down(Key::PageDown))
      {
        m_camera.move_down(move_speed);
      }
    }

    game.on_update(*this, m_dt);

    if (m_framebuffer.width() != m_window->width() || m_framebuffer.height() != m_window->height())
    {
      m_framebuffer.resize(m_window->width(), m_window->height());
      m_camera.set_aspect(static_cast<float>(m_window->width()) / m_window->height());
    }

    resolve_lights_into(m_scene.lights, m_camera, m_resolved_lights);
    if (m_config.headlamp)
    {
      m_resolved_lights.push_back(LightF::directional(m_camera.forward()));
    }
    const auto gpu_active = m_gpu != nullptr && m_gpu->ok();
    if (gpu_active)
    {
      // GPU renders the 3D scene; the CPU framebuffer becomes a transparent
      // overlay layer (on_render/HUD), composited on top afterwards.
      m_stats = RenderStats{};
      m_gpu->render(m_scene, m_camera, m_resolved_lights, m_config.ambient, m_wireframe,
                    m_config.clear_color, m_config.texture_filter, m_config.fog, m_window->width(),
                    m_window->height(), m_config.sky_gradient ? &m_config.sky_top : nullptr,
                    m_config.sky_gradient ? &m_config.sky_bottom : nullptr);
      m_framebuffer.clear(Color{0, 0, 0, 0});
    }
    else
    {
      if (m_config.sky_gradient)
      {
        clear_frame_gradient(m_framebuffer, m_config.sky_top, m_config.sky_bottom);
      }
      else
      {
        clear_frame(m_framebuffer, m_config.clear_color);
      }
      m_stats = render_scene(m_framebuffer, m_scene, m_camera, m_resolved_lights, m_config.ambient,
                             m_wireframe, m_config.texture_filter, m_config.fog);
    }

    game.on_render(*this);

    if (m_show_hud)
    {
      const auto hud = overlay::HudInput{&m_scene,    &m_stats, &m_resolved_lights,
                                         &m_camera,   fps(),    m_frame_ms,
                                         m_wireframe, 0,        false,
                                         -1,          "game",   {}};
      overlay::draw_hud(m_framebuffer, hud);
    }

    if (gpu_active)
    {
      m_gpu->draw_overlay(m_framebuffer);
      m_window->swap_buffers();
    }
    else
    {
      m_window->present(m_framebuffer);
    }
  }

  game.on_shutdown(*this);
  m_window->set_relative_mouse(false);
  return 0;
}

} // namespace obzcene
