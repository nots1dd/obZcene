/**
 * @file Viewer.cpp
 * @brief Interactive viewer frame loop, scene editing and input handling.
 *
 * Implements the controls documented by Viewer/Viewer.hpp:
 *
 * Fly mode (default): WASDQE + PgUp/PgDn camera movement, mouse look,
 * R = reset pose, F1 = wireframe, F2 = stats, F3 = headlamp, `[`/`]` = cycle
 * the movable light, arrows/Home/End = move that light, Tab = cycle selected
 * object, G = grab (move) the selected object, Del = remove it, F4 = open the
 * file browser to add an object, F5 = save scene, F9 = reload scene file.
 *
 * Grab mode: arrows/Home/End move the grabbed object (camera plane), PgUp/PgDn
 * move it up/down, Enter confirms, Escape cancels (restores the transform).
 *
 * Browse mode: Up/Down move the cursor, Enter opens directories / picks a
 * file, Backspace goes to the parent directory, F4/Escape closes.
 *
 * Each frame it resolves the lights (camera binding), clears, rasterizes
 * every scene object, draws the HUD + browser overlay and presents via
 * obzcene::Window.
 *
 * @see Viewer/Viewer.hpp (declaration), Renderer/Window.hpp,
 *      Viewer/Overlay.hpp, Viewer/Scene.hpp, Viewer/SceneFile.hpp
 */

#include "obzcene/Viewer/Viewer.hpp"

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Renderer/Rasterizer.hpp"
#include "obzcene/Renderer/Shader.hpp"
#include "obzcene/Renderer/Window.hpp"
#include "obzcene/Viewer/Overlay.hpp"
#include "obzcene/Viewer/SceneFile.hpp"

#include <SDL.h>

namespace obzcene
{

namespace
{

constexpr float kMouseSensitivity = 0.0025F;
constexpr float kMoveMultiplier   = 2.0F;
constexpr float kGrabMultiplier   = 1.0F;
constexpr float kRotateSpeed      = 1.5F; ///< radians per second while grabbing
constexpr float kScaleSpeed       = 0.5F; ///< scale factor per second while grabbing

} // namespace

Viewer::Viewer(std::string title, int width, int height)
    : m_window(std::make_unique<Window>(std::move(title), width, height)),
      m_framebuffer(width, height)
{
}

Viewer::~Viewer() = default;

auto Viewer::set_message(std::string message) -> void
{
  m_message     = std::move(message);
  m_message_ttl = 3.0F;
}

auto Viewer::cycle_selection(int step) -> void
{
  if (m_scene.objects.empty())
  {
    m_selected = -1;
    return;
  }
  const auto count = static_cast<int>(m_scene.objects.size());
  auto       next  = m_selected + step;
  while (next < 0)
  {
    next += count;
  }
  m_selected = next % count;
}

auto Viewer::save() -> void
{
  const auto path = m_scene_path.empty() ? std::string{"scene.obzscene"} : m_scene_path;
  if (save_scene(m_scene, path, m_camera))
  {
    set_message("saved " + path);
  }
  else
  {
    set_message("save FAILED: " + path);
  }
}

auto Viewer::reload() -> void
{
  if (m_scene_path.empty())
  {
    set_message("no scene file to reload");
    return;
  }
  const auto data = load_scene_file(m_scene_path);
  if (!data.has_value())
  {
    set_message("load FAILED: " + m_scene_path);
    return;
  }
  const auto scene_dir = std::filesystem::path{m_scene_path}.parent_path();
  apply_scene_file(m_scene, *data, scene_dir.empty() ? "." : scene_dir, m_no_textures);
  m_lights   = m_scene.lights;
  m_selected = m_scene.objects.empty() ? -1 : 0;
  if (m_active_light >= m_lights.size())
  {
    m_active_light = 0;
  }
  if (data->camera_position.has_value())
  {
    m_camera.set_position(*data->camera_position);
    m_camera.set_orientation(data->camera_yaw.value_or(0.0F), data->camera_pitch.value_or(0.0F));
  }
  set_message("loaded " + m_scene_path);
}

auto Viewer::add_object_from_browser(const std::filesystem::path& path) -> void
{
  auto model = m_scene.load_model(path, m_no_textures);
  if (model == nullptr)
  {
    set_message("failed to load " + path.string());
    return;
  }
  // Spawn a bit in front of the camera so the user sees it immediately,
  // then hand it to grab mode for placement.
  const auto radius =
    std::max(math::length(model->mesh.bounds.max - model->mesh.bounds.min) * 0.5F, 0.5F);
  const auto spawn   = m_camera.position() + m_camera.forward() * (radius * 2.0F + 1.0F);
  m_selected         = narrow_cast<int>(m_scene.add_object(std::move(model), spawn));
  auto& object       = m_scene.objects[static_cast<usz>(m_selected)];
  object.name        = path.filename().string();
  object.source_path = path;
  m_mode             = ViewerMode::Grab;
  set_message("added " + path.filename().string());
}

auto Viewer::handle_toggles() -> void
{

  const auto wireframe_down = m_window->key(SDL_SCANCODE_F1);
  if (wireframe_down && !m_prev_wireframe)
  {
    m_wireframe = !m_wireframe;
  }
  m_prev_wireframe = wireframe_down;

  const auto stats_down = m_window->key(SDL_SCANCODE_F2);
  if (stats_down && !m_prev_stats)
  {
    m_show_stats = !m_show_stats;
  }
  m_prev_stats = stats_down;

  const auto headlamp_down = m_window->key(SDL_SCANCODE_F3);
  if (headlamp_down && !m_prev_headlamp)
  {
    m_headlamp = !m_headlamp;
  }
  m_prev_headlamp = headlamp_down;

  const auto browser_down = m_window->key(SDL_SCANCODE_F4);
  if (browser_down && !m_prev_browser)
  {
    if (m_mode != ViewerMode::Browse)
    {
      m_browser.open(m_scene_path.empty() ? std::filesystem::current_path()
                                          : std::filesystem::path{m_scene_path}.parent_path(),
                     ".obj");
      m_mode = ViewerMode::Browse;
    }
    else
    {
      m_browser.close();
      m_mode = ViewerMode::Fly;
    }
  }
  m_prev_browser = browser_down;

  const auto save_down = m_window->key(SDL_SCANCODE_F5);
  if (save_down && !m_prev_save)
  {
    save();
  }
  m_prev_save = save_down;

  const auto load_down = m_window->key(SDL_SCANCODE_F9);
  if (load_down && !m_prev_load)
  {
    reload();
  }
  m_prev_load = load_down;
}

auto Viewer::handle_light_move(float move_speed) -> void
{
  if (m_lights.empty() || m_lights[m_active_light].type == LightType::Directional)
  {
    return;
  }
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

auto Viewer::handle_fly_input(float dt) -> void
{
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

  // Tab cycles the selected object; G starts a grab; Del removes it.
  const auto tab_down = m_window->key(SDL_SCANCODE_TAB);
  if (tab_down && !m_prev_tab)
  {
    cycle_selection(1);
  }
  m_prev_tab = tab_down;

  const auto grab_down = m_window->key(SDL_SCANCODE_G);
  if (grab_down && !m_prev_grab && m_selected >= 0 &&
      static_cast<usz>(m_selected) < m_scene.objects.size())
  {
    m_mode = ViewerMode::Grab;
    set_message("grab: arrows/Home/End move, PgUp/PgDn height, enter to confirm");
  }
  m_prev_grab = grab_down;

  const auto delete_down = m_window->key(SDL_SCANCODE_DELETE);
  if (delete_down && !m_prev_delete && m_selected >= 0)
  {
    const auto name = m_scene.objects[static_cast<usz>(m_selected)].name;
    m_scene.remove_object(static_cast<usz>(m_selected));
    if (m_selected >= static_cast<int>(m_scene.objects.size()))
    {
      m_selected = static_cast<int>(m_scene.objects.size()) - 1;
    }
    set_message("removed " + name);
  }
  m_prev_delete = delete_down;

  const auto bracket_down =
    m_window->key(SDL_SCANCODE_LEFTBRACKET) || m_window->key(SDL_SCANCODE_RIGHTBRACKET);
  if (bracket_down && !m_prev_bracket && !m_lights.empty())
  {
    const auto step = m_window->key(SDL_SCANCODE_RIGHTBRACKET) ? 1 : -1;
    for (usz n = 0; n < m_lights.size(); ++n)
    {
      auto idx = (static_cast<int>(m_active_light) + step) % static_cast<int>(m_lights.size());
      if (idx < 0)
      {
        idx += static_cast<int>(m_lights.size());
      }
      if (m_lights[static_cast<usz>(idx)].type != LightType::Directional)
      {
        m_active_light = static_cast<usz>(idx);
        break;
      }
      m_active_light = static_cast<usz>(idx);
    }
  }
  m_prev_bracket = bracket_down;

  handle_light_move(move_speed);

  if (m_window->key(SDL_SCANCODE_ESCAPE))
  {
    m_window->close();
  }
}

auto Viewer::handle_grab_input(float dt) -> void
{
  if (m_selected < 0 || static_cast<usz>(m_selected) >= m_scene.objects.size())
  {
    m_mode = ViewerMode::Fly;
    return;
  }
  auto&      object = m_scene.objects[static_cast<usz>(m_selected)];
  const auto speed  = dt * m_move_speed * kGrabMultiplier;

  const auto plane_forward = [&]()
  {
    auto f = m_camera.forward();
    f.y    = 0.0F;
    return math::normalized(f);
  };
  if (m_window->key(SDL_SCANCODE_UP))
  {
    object.position += plane_forward() * speed;
  }
  if (m_window->key(SDL_SCANCODE_DOWN))
  {
    object.position -= plane_forward() * speed;
  }
  if (m_window->key(SDL_SCANCODE_LEFT))
  {
    object.position -= m_camera.right() * speed;
  }
  if (m_window->key(SDL_SCANCODE_RIGHT))
  {
    object.position += m_camera.right() * speed;
  }
  if (m_window->key(SDL_SCANCODE_HOME))
  {
    object.position += math::Vec3f{0, 1, 0} * speed;
  }
  if (m_window->key(SDL_SCANCODE_END))
  {
    object.position -= math::Vec3f{0, 1, 0} * speed;
  }
  if (m_window->key(SDL_SCANCODE_PAGEUP))
  {
    object.yaw += kRotateSpeed * dt;
  }
  if (m_window->key(SDL_SCANCODE_PAGEDOWN))
  {
    object.yaw -= kRotateSpeed * dt;
  }
  if (m_window->key(SDL_SCANCODE_EQUALS))
  {
    object.scale += kScaleSpeed * dt;
  }
  if (m_window->key(SDL_SCANCODE_MINUS))
  {
    object.scale = std::max(0.05F, object.scale - kScaleSpeed * dt);
  }

  // Enter confirms; Escape cancels (restores the saved transform).
  if (m_window->key(SDL_SCANCODE_RETURN))
  {
    m_mode = ViewerMode::Fly;
    set_message("placed " + object.name);
    return;
  }
  if (m_window->key(SDL_SCANCODE_ESCAPE))
  {
    m_mode = ViewerMode::Fly;
    set_message("grab cancelled");
  }
}

auto Viewer::handle_browse_input() -> void
{
  const auto up_down = m_window->key(SDL_SCANCODE_UP);
  if (up_down && !m_prev_browse_up)
  {
    m_browser.move_cursor(-1);
  }
  m_prev_browse_up = up_down;

  const auto down_down = m_window->key(SDL_SCANCODE_DOWN);
  if (down_down && !m_prev_browse_down)
  {
    m_browser.move_cursor(1);
  }
  m_prev_browse_down = down_down;

  const auto enter_down = m_window->key(SDL_SCANCODE_RETURN);
  if (enter_down && !m_prev_browse_enter)
  {
    m_browser.activate();
    if (!m_browser.is_open())
    {
      m_mode              = ViewerMode::Fly;
      const auto selected = m_browser.take_selected();
      if (selected.has_value())
      {
        add_object_from_browser(*selected);
      }
    }
  }
  m_prev_browse_enter = enter_down;

  const auto back_down = m_window->key(SDL_SCANCODE_BACKSPACE);
  if (back_down && !m_prev_browse_back)
  {
    m_browser.go_parent();
  }
  m_prev_browse_back = back_down;

  if (m_window->key(SDL_SCANCODE_ESCAPE))
  {
    m_browser.close();
    m_mode = ViewerMode::Fly;
  }
}

auto Viewer::run(Scene scene, const std::string& scene_path, bool no_textures) -> void
{
  if (!m_window->is_open())
  {
    return;
  }
  m_window->set_relative_mouse(true);
  m_scene       = std::move(scene);
  m_scene_path  = scene_path;
  m_no_textures = no_textures;
  m_lights      = m_scene.lights;
  m_selected    = m_scene.objects.empty() ? -1 : 0;

  // Pick the first movable (non-directional) light.
  if (!m_lights.empty() && m_lights[m_active_light].type == LightType::Directional)
  {
    for (usz i = 0; i < m_lights.size(); ++i)
    {
      if (m_lights[i].type != LightType::Directional)
      {
        m_active_light = i;
        break;
      }
    }
  }

  auto last_frame = SDL_GetTicks64();
  while (m_window->is_open())
  {
    const auto now      = SDL_GetTicks64();
    const auto dt       = static_cast<float>(now - last_frame) / 1000.0F;
    last_frame          = now;
    const auto frame_ms = dt * 1000.0F;
    m_frame_ms          = m_frame_ms * 0.9F + frame_ms * 0.1F;

    if (m_message_ttl > 0.0F)
    {
      m_message_ttl -= dt;
      if (m_message_ttl <= 0.0F)
      {
        m_message.clear();
      }
    }

    m_window->handle_events();
    handle_toggles();

    switch (m_mode)
    {
      case ViewerMode::Fly:
        handle_fly_input(dt);
        break;
      case ViewerMode::Grab:
        handle_grab_input(dt);
        break;
      case ViewerMode::Browse:
        handle_browse_input();
        break;
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

    clear_frame(m_framebuffer);
    auto render_stats = RenderStats{};
    for (const auto& object : m_scene.objects)
    {
      if (object.model == nullptr)
      {
        continue;
      }
      const auto bounds    = object.transformed_bounds(object.model->mesh.bounds);
      const auto transform = ModelTransform{object.model_matrix(), object.normal_matrix()};
      const auto uniforms =
        FragmentUniforms{m_camera, m_resolved_lights, object.model->textures, m_ambient_intensity};
      const auto sub =
        render_object(m_framebuffer, object.model->mesh, uniforms, m_wireframe, transform, &bounds);
      render_stats.triangles_processed += sub.triangles_processed;
      render_stats.triangles_culled_backface += sub.triangles_culled_backface;
      render_stats.triangles_culled_viewport += sub.triangles_culled_viewport;
      render_stats.triangles_culled_nearfar += sub.triangles_culled_nearfar;
      render_stats.triangles_degenerate += sub.triangles_degenerate;
      render_stats.triangles_rendered += sub.triangles_rendered;
      render_stats.pixels_shaded += sub.pixels_shaded;
      render_stats.pixels_written += sub.pixels_written;
    }

    if (m_show_stats)
    {
      const auto hud = overlay::HudInput{&m_scene,
                                         &render_stats,
                                         &m_resolved_lights,
                                         &m_camera,
                                         m_frame_ms > 0.0F ? 1000.0F / m_frame_ms : 0.0F,
                                         m_frame_ms,
                                         m_wireframe,
                                         m_active_light,
                                         m_headlamp,
                                         m_selected,
                                         m_mode == ViewerMode::Fly    ? "fly"
                                         : m_mode == ViewerMode::Grab ? "grab"
                                                                      : "browse",
                                         m_message};
      overlay::draw_hud(m_framebuffer, hud);
    }
    if (m_mode == ViewerMode::Browse)
    {
      overlay::draw_browser(m_framebuffer, m_browser);
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
