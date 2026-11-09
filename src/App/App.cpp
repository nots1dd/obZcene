/**
 * @file App.cpp
 * @brief Application entry: CLI parsing, mode dispatch, snapshot rendering.
 *
 * Uses CLI11 for argument handling. Two modes: interactive (opens the
 * obzcene::Viewer) and snapshot (forces SDL to the dummy video driver, renders
 * one frame via obzcene::app::render_snapshot to a PPM and exits). The SDL
 * init here exists only so SDL_image/IMG_Load in the texture path has a video
 * subsystem even in headless mode.
 *
 * @see obzcene::app::entry (declaration),
 *      App/Scene.hpp (load_scene/render_snapshot), Viewer/Viewer.hpp
 */

#include "obzcene/App/App.hpp"

#include "obzcene/App/Scene.hpp"
#include "obzcene/Core/Logger.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Viewer/Viewer.hpp"

#include <CLI/CLI.hpp>
#include <SDL.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace obzcene::app
{

namespace
{

/** @brief Wire a loaded scene into the interactive viewer and run it. */
auto run_viewer(const Scene& scene) -> void
{
  Viewer viewer{"obZcene", scene.options.width, scene.options.height};
  setup_camera(viewer.camera(), scene.view, scene.options.width, scene.options.height,
               scene.options.fov_y);
  viewer.set_move_speed(scene.view.speed);
  viewer.set_reset_pose(scene.view.eye, scene.view.center);
  viewer.set_wireframe(scene.options.wireframe);
  viewer.set_headlamp(scene.options.headlamp);
  viewer.set_ambient(scene.options.ambient);
  auto scene_copy = scene.scene; // the Viewer edits its own copy
  viewer.run(std::move(scene_copy), scene.scene_file.string(), scene.options.no_textures);
}

} // namespace

auto entry(int argc, char** argv) -> int
{
  Logger::init();

  auto        scene_options = SceneOptions{};
  std::string snapshot_path;

  CLI::App cli{"obZcene - software rasterizer viewer"};
  cli.add_option("model", scene_options.model, "Path to a Wavefront .obj model")
    ->check(CLI::ExistingFile);
  cli.add_option("--scene", scene_options.scene_file, "Load a .obzscene scene file");
  cli.add_option("-s,--snapshot", snapshot_path, "Render a single frame to a PPM file and exit");
  cli.add_option("-W,--width", scene_options.width, "Output width");
  cli.add_option("-H,--height", scene_options.height, "Output height");
  cli.add_option("--fov", scene_options.fov_y, "Vertical field of view in degrees");
  cli.add_flag("-w,--wireframe", scene_options.wireframe, "Start in wireframe mode");
  cli.add_flag("--no-textures", scene_options.no_textures, "Disable texture loading");
  auto        light_pos       = std::vector<float>{};
  auto        light_dir       = std::vector<float>{};
  auto        light_color     = std::vector<float>{1.0F, 1.0F, 1.0F};
  auto        light_intensity = 1.0F;
  const auto* pos_opt =
    cli.add_option("--light-pos", light_pos, "Place a point or spot light at a world position")
      ->expected(3);
  const auto* dir_opt =
    cli.add_option("--light-dir", light_dir, "Directional light direction, or spot light aim")
      ->expected(3);
  cli.add_option("--light-color", light_color, "Light color as RGB in 0..1")->expected(3);
  cli.add_option("--light-intensity", light_intensity, "Light intensity");
  cli.add_flag("--headlamp", scene_options.headlamp, "Attach a directional headlamp to the camera");
  cli.add_option("--ambient", scene_options.ambient, "Ambient light intensity");
  CLI11_PARSE(cli, argc, argv);

  if (pos_opt->count() > 0)
  {
    scene_options.light_position = math::Vec3f{light_pos[0], light_pos[1], light_pos[2]};
  }
  if (dir_opt->count() > 0)
  {
    scene_options.light_direction = math::Vec3f{light_dir[0], light_dir[1], light_dir[2]};
  }
  scene_options.light_color     = math::Vec3f{light_color[0], light_color[1], light_color[2]};
  scene_options.light_intensity = light_intensity;

  const auto snapshot_only = !snapshot_path.empty();
  if (snapshot_only)
  {
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
      LOG_WARN("SDL video init failed: {}", SDL_GetError());
    }
  }

  auto scene = load_scene(scene_options);
  if (!scene.has_value())
  {
    Logger::shutdown();
    if (snapshot_only)
    {
      SDL_Quit();
    }
    return 1;
  }

  auto result = 0;
  if (snapshot_only)
  {
    if (!render_snapshot(*scene, snapshot_path))
    {
      LOG_ERROR("Failed to write snapshot '{}'", snapshot_path);
      result = 1;
    }
    else
    {
      LOG_INFO("Snapshot written to '{}'", snapshot_path);
    }
  }
  else
  {
    run_viewer(*scene);
  }

  Logger::shutdown();
  if (snapshot_only)
  {
    SDL_Quit();
  }
  return result;
}

} // namespace obzcene::app
