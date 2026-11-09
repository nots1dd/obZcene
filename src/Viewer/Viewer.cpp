/**
 * @file Viewer.cpp
 * @brief Interactive viewer frame loop and input handling.
 *
 * Implements the controls documented by Viewer/Viewer.hpp: WASDQE camera
 * movement, PgUp/PgDn for vertical movement, mouse look, R = reset pose,
 * F1 = wireframe, F2 = stats panel, F3 = headlamp, `[`/`]` = cycle the
 * movable light, and arrows/Home/End to move that light around. Each frame
 * it resolves the lights (camera binding), clears, rasterizes, draws the
 * overlay and presents via obzcene::Window.
 *
 * @see Viewer/Viewer.hpp (declaration), Renderer/Window.hpp,
 *      Viewer/Overlay.hpp
 */

#include "obzcene/Viewer/Viewer.hpp"

#include "obzcene/Renderer/Rasterizer.hpp"
#include "obzcene/Renderer/Shader.hpp"
#include "obzcene/Renderer/Window.hpp"
#include "obzcene/Viewer/Overlay.hpp"

#include <SDL.h>

namespace obzcene
{

namespace
{

constexpr float kMouseSensitivity = 0.0025F;
constexpr float kMoveMultiplier   = 2.0F;

} // namespace

Viewer::Viewer(std::string title, int width, int height)
    : m_window(std::make_unique<Window>(std::move(title), width, height)),
      m_framebuffer(width, height)
{
}

Viewer::~Viewer() = default;

auto Viewer::run(const Mesh& mesh, const std::vector<LightF>& lights,
                 const std::vector<Texture>& textures) -> void
{
  if (!m_window->is_open())
  {
    return;
  }
  m_window->set_relative_mouse(true);
  m_lights = lights;

  auto last_frame         = SDL_GetTicks64();
  auto wireframe_was_down = false;
  auto stats_was_down     = false;
  auto headlamp_was_down  = false;
  auto bracket_was_down   = false;

  const auto pick_active_light = [&](usz from, int step) -> usz
  {
    if (m_lights.empty())
    {
      return 0;
    }
    for (usz n = 0; n < m_lights.size(); ++n)
    {
      auto idx = (static_cast<int>(from) + step) % static_cast<int>(m_lights.size());
      if (idx < 0)
      {
        idx += static_cast<int>(m_lights.size());
      }
      if (m_lights[static_cast<usz>(idx)].type != LightType::Directional)
      {
        return static_cast<usz>(idx);
      }
      from = static_cast<usz>(idx);
    }
    return from;
  };
  m_active_light = pick_active_light(m_active_light, 1);

  while (m_window->is_open())
  {
    const auto now      = SDL_GetTicks64();
    const auto dt       = static_cast<float>(now - last_frame) / 1000.0F;
    last_frame          = now;
    const auto frame_ms = dt * 1000.0F;
    m_frame_ms          = m_frame_ms * 0.9F + frame_ms * 0.1F;

    m_window->handle_events();

    const auto mouse = m_window->take_mouse_delta();
    m_camera.yaw_by(static_cast<float>(-mouse.first) * kMouseSensitivity);
    m_camera.pitch_by(static_cast<float>(-mouse.second) * kMouseSensitivity);

    const auto move_speed = dt * m_move_speed * kMoveMultiplier;
    if (m_window->key(SDL_SCANCODE_W))
    {
      m_camera.move_forward(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_S))
    {
      m_camera.move_backward(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_A))
    {
      m_camera.move_left(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_D))
    {
      m_camera.move_right(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_E))
    {
      m_camera.move_up(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_Q))
    {
      m_camera.move_down(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_PAGEUP))
    {
      m_camera.move_up(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_PAGEDOWN))
    {
      m_camera.move_down(move_speed);
    }
    if (m_window->key(SDL_SCANCODE_R))
    {
      m_camera.look_at(m_reset_eye, m_reset_target);
    }

    const auto wireframe_down = m_window->key(SDL_SCANCODE_F1);
    if (wireframe_down && !wireframe_was_down)
    {
      m_wireframe = !m_wireframe;
    }
    wireframe_was_down = wireframe_down;

    const auto stats_down = m_window->key(SDL_SCANCODE_F2);
    if (stats_down && !stats_was_down)
    {
      m_show_stats = !m_show_stats;
    }
    stats_was_down = stats_down;

    const auto headlamp_down = m_window->key(SDL_SCANCODE_F3);
    if (headlamp_down && !headlamp_was_down)
    {
      m_headlamp = !m_headlamp;
    }
    headlamp_was_down = headlamp_down;

    const auto bracket_down =
      m_window->key(SDL_SCANCODE_LEFTBRACKET) || m_window->key(SDL_SCANCODE_RIGHTBRACKET);
    if (bracket_down && !bracket_was_down)
    {
      const auto step = m_window->key(SDL_SCANCODE_RIGHTBRACKET) ? 1 : -1;
      m_active_light  = pick_active_light(m_active_light, step);
    }
    bracket_was_down = bracket_down;

    if (!m_lights.empty() && m_lights[m_active_light].type != LightType::Directional)
    {
      auto&      active_light  = m_lights[m_active_light];
      const auto plane_forward = [&]()
      {
        auto f = m_camera.forward();
        f.y    = 0.0F;
        return math::normalized(f);
      };
      const auto light_speed = move_speed * 0.5F;
      if (m_window->key(SDL_SCANCODE_UP))
      {
        active_light.position += plane_forward() * light_speed;
      }
      if (m_window->key(SDL_SCANCODE_DOWN))
      {
        active_light.position -= plane_forward() * light_speed;
      }
      if (m_window->key(SDL_SCANCODE_LEFT))
      {
        active_light.position -= m_camera.right() * light_speed;
      }
      if (m_window->key(SDL_SCANCODE_RIGHT))
      {
        active_light.position += m_camera.right() * light_speed;
      }
      if (m_window->key(SDL_SCANCODE_HOME))
      {
        active_light.position += math::Vec3f{0, 1, 0} * light_speed;
      }
      if (m_window->key(SDL_SCANCODE_END))
      {
        active_light.position -= math::Vec3f{0, 1, 0} * light_speed;
      }
    }

    if (m_framebuffer.width() != m_window->width() || m_framebuffer.height() != m_window->height())
    {
      m_framebuffer.resize(m_window->width(), m_window->height());
      m_camera.set_aspect(static_cast<float>(m_window->width()) / m_window->height());
    }

    resolve_lights_into(m_lights, m_camera, m_resolved_lights);
    if (m_headlamp)
    {
      m_resolved_lights.push_back(LightF::directional(m_camera.forward()));
    }
    const auto uniforms =
      FragmentUniforms{m_camera, m_resolved_lights, textures, m_ambient_intensity};
    const auto render_stats = render_frame(m_framebuffer, mesh, uniforms, m_wireframe);
    if (m_show_stats)
    {
      overlay::draw_stats(m_framebuffer, render_stats, mesh, textures, m_resolved_lights, m_camera,
                          m_frame_ms > 0.0F ? 1000.0F / m_frame_ms : 0.0F, m_frame_ms, m_wireframe,
                          m_active_light, m_headlamp);
    }

    m_window->present(m_framebuffer);
  }

  m_window->set_relative_mouse(false);
}

auto Viewer::camera() -> CameraF& { return m_camera; }

auto Viewer::camera() const -> const CameraF& { return m_camera; }

auto Viewer::set_move_speed(float speed) -> void { m_move_speed = speed; }

auto Viewer::set_reset_pose(const math::Vec3f& eye, const math::Vec3f& target) -> void
{
  m_reset_eye    = eye;
  m_reset_target = target;
  m_camera.look_at(eye, target);
}

auto Viewer::set_wireframe(bool enabled) -> void { m_wireframe = enabled; }

auto Viewer::set_headlamp(bool enabled) -> void { m_headlamp = enabled; }

auto Viewer::set_ambient(float intensity) -> void { m_ambient_intensity = intensity; }

} // namespace obzcene