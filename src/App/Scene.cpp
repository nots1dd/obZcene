/**
 * @file Scene.cpp
 * @brief Scene assembly and the single-frame (snapshot) render path.
 *
 * This TU implements the obzcene::app helpers declared in App/Scene.hpp:
 * computing a camera framing from the mesh bounds, building the light list,
 * loading the mesh/textures, and rendering one frame to a PPM. It is the
 * bridge between the parsers (File/obj...) + texture loader and the
 * rasterizer (Renderer/Rasterizer.hpp).
 *
 * @see App/Scene.hpp (declarations), Renderer/Rasterizer.hpp,
 *      Texture/ImageWriter.hpp
 */

#include "obzcene/App/Scene.hpp"

#include "obzcene/Core/Logger.hpp"
#include "obzcene/Renderer/Color.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Rasterizer.hpp"
#include "obzcene/Renderer/Shader.hpp"
#include "obzcene/Texture/ImageWriter.hpp"
#include "obzcene/Viewer/SceneFile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace obzcene::app
{

auto compute_scene_view(const math::AABBf& bounds, int width, int height, float fov_y) -> SceneView
{
  const auto center     = (bounds.max + bounds.min) * 0.5F;
  const auto radius     = std::max(math::length(bounds.max - bounds.min) * 0.5F, 1.0e-3F);
  const auto half       = (bounds.max - bounds.min) * 0.5F;
  const auto aspect     = std::max(static_cast<float>(width) / height, 1.0F);
  const auto half_fov   = math::deg_to_rad(fov_y) * 0.5F;
  const auto fit_half   = std::max(half.y, math::length(math::Vec2f{half.x, half.z}) / aspect);
  const auto fit        = (half.z + fit_half / std::tan(half_fov)) * 1.1F;
  const auto eye_offset = math::Vec3f{0, radius * 0.3F, fit};
  return SceneView{.center     = center,
                   .eye        = center + eye_offset,
                   .eye_offset = eye_offset,
                   .radius     = radius,
                   .speed      = std::max(radius * 0.25F, 0.05F)};
}

auto setup_camera(CameraF& camera, const SceneView& view, int width, int height, float fov_y)
  -> void
{
  camera.set_perspective(math::deg_to_rad(fov_y), static_cast<float>(width) / height, 0.05F,
                         view.radius * 40.0F + 100.0F);
  camera.look_at(view.eye, view.center);
}

auto make_lights(const SceneView& view, const SceneOptions& options) -> std::vector<LightF>
{
  auto lights = std::vector<LightF>{};
  if (options.light_position.has_value() || options.light_direction.has_value())
  {
    const auto dir_used = options.light_direction.has_value();
    const auto dir      = dir_used ? options.light_direction->normalized() : math::Vec3f{};
    if (dir_used)
    {
      lights.push_back(LightF::directional(dir, options.light_color, options.light_intensity));
    }
    if (options.light_position.has_value())
    {
      lights.push_back(
        dir_used
          ? LightF::spot(*options.light_position, dir, options.light_color, options.light_intensity)
          : LightF::point(*options.light_position, options.light_color, options.light_intensity));
    }
    return lights;
  }

  auto sun  = LightF::directional(math::Vec3f{-0.35F, -1.0F, -0.35F}.normalized(),
                                  math::Vec3f{1, 0.98F, 0.9F}, 1.0F);
  auto lamp = LightF::point(view.center + math::Vec3f{0, view.radius * 2.0F, view.radius * 1.5F},
                            math::Vec3f{0.6F, 0.7F, 1.0F}, 1.0F);
  lamp.quadratic = 0.5F / (view.radius * view.radius);
  auto fill =
    LightF::point(view.center + math::Vec3f{-view.radius, -view.radius * 0.5F, -view.radius},
                  math::Vec3f{0.7F, 0.8F, 1.0F}, 0.4F);
  fill.quadratic = 0.8F / (view.radius * view.radius);
  return std::vector<LightF>{sun, lamp, fill};
}

auto load_scene(const SceneOptions& options) -> std::optional<Scene>
{
  auto scene       = Scene{};
  scene.options    = options;
  scene.scene_file = options.scene_file;

  // A scene file, when given, loads first; the positional model is added on
  // top so both can be combined.
  if (!options.scene_file.empty())
  {
    const auto data = load_scene_file(options.scene_file);
    if (!data.has_value())
    {
      LOG_ERROR("Failed to open scene file '{}'", options.scene_file.string());
      return std::nullopt;
    }
    const auto scene_dir = options.scene_file.has_parent_path() ? options.scene_file.parent_path()
                                                                : std::filesystem::path{"."};
    apply_scene_file(scene.scene, *data, scene_dir, options.no_textures);
    if (data->camera_position.has_value())
    {
      scene.saved_camera_position = data->camera_position;
      scene.saved_camera_yaw      = data->camera_yaw.value_or(0.0F);
      scene.saved_camera_pitch    = data->camera_pitch.value_or(0.0F);
    }
  }

  if (!options.model.empty())
  {
    auto model = scene.scene.load_model(options.model, options.no_textures);
    if (model == nullptr)
    {
      LOG_ERROR("Failed to load model '{}'", options.model.string());
      return std::nullopt;
    }
    const auto index   = scene.scene.add_object(std::move(model), math::Vec3f{0.0F});
    auto&      object  = scene.scene.objects[index];
    object.name        = options.model.filename().string();
    object.source_path = options.model;
  }

  if (scene.scene.objects.empty())
  {
    LOG_ERROR("Nothing to render: no model and no scene file produced any objects");
    return std::nullopt;
  }

  scene.view =
    compute_scene_view(scene.scene.world_bounds(), options.width, options.height, options.fov_y);
  if (scene.saved_camera_position.has_value())
  {
    // The framing still provides movement speed and the reset pose; the saved
    // pose wins for the actual starting camera.
    scene.view.eye = *scene.saved_camera_position;
    const auto dir =
      math::Vec3f{-std::sin(scene.saved_camera_yaw) * std::cos(scene.saved_camera_pitch),
                  std::sin(scene.saved_camera_pitch),
                  -std::cos(scene.saved_camera_yaw) * std::cos(scene.saved_camera_pitch)};
    scene.view.center = scene.view.eye + dir;
  }
  if (scene.scene.lights.empty())
  {
    scene.scene.lights = make_lights(scene.view, options);
  }
  return scene;
}

auto render_snapshot(const Scene& scene, const std::string& path) -> bool
{
  CameraF camera;
  setup_camera(camera, scene.view, scene.options.width, scene.options.height, scene.options.fov_y);
  auto frame_buffer  = FrameBuffer{scene.options.width, scene.options.height};
  auto render_lights = resolve_lights(scene.scene.lights, camera);
  if (scene.options.headlamp)
  {
    render_lights.push_back(LightF::directional(camera.forward()));
  }
  clear_frame(frame_buffer);
  const auto t0    = std::chrono::steady_clock::now();
  auto       stats = RenderStats{};
  for (const auto& object : scene.scene.objects)
  {
    if (object.model == nullptr)
    {
      continue;
    }
    const auto bounds    = object.transformed_bounds(object.model->mesh.bounds);
    const auto transform = ModelTransform{object.model_matrix(), object.normal_matrix()};
    const auto uniforms =
      FragmentUniforms{camera, render_lights, object.model->textures, scene.options.ambient};
    const auto sub = render_object(frame_buffer, object.model->mesh, uniforms,
                                   scene.options.wireframe, transform, &bounds);
    stats.triangles_processed += sub.triangles_processed;
    stats.triangles_culled_backface += sub.triangles_culled_backface;
    stats.triangles_culled_viewport += sub.triangles_culled_viewport;
    stats.triangles_culled_nearfar += sub.triangles_culled_nearfar;
    stats.triangles_degenerate += sub.triangles_degenerate;
    stats.triangles_rendered += sub.triangles_rendered;
    stats.pixels_shaded += sub.pixels_shaded;
    stats.pixels_written += sub.pixels_written;
  }
  const auto elapsed_ms =
    std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count();
  LOG_INFO("Rasterized in {:.1f} ms: {} tris processed, {} drawn (bf {}, vp {}, near {}, deg {}), "
           "{} px shaded, {} px written",
           elapsed_ms, stats.triangles_processed, stats.triangles_rendered,
           stats.triangles_culled_backface, stats.triangles_culled_viewport,
           stats.triangles_culled_nearfar, stats.triangles_degenerate, stats.pixels_shaded,
           stats.pixels_written);
  return write_ppm(path, frame_buffer);
}

} // namespace obzcene::app
