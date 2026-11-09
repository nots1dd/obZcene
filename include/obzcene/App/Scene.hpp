#pragma once

/**
 * @file Scene.hpp
 * @brief Load-time scene assembly: mesh, textures, lights and the initial
 *        camera framing.
 *
 * These helpers turn CLI options (obzcene::app::SceneOptions) into a ready
 * chunk of renderable state (obzcene::app::Scene). `load_scene` does the
 * parse/build/texture/light work; `compute_scene_view` + `setup_camera` frame
 * the camera around the model so it is visible on the first frame; and
 * `render_snapshot` runs the headless single-frame path.
 *
 * @see App/App.hpp (entry drives these), Viewer/Viewer.hpp
 */

#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/Mesh.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace obzcene::app
{

/**
 * @brief Precomputed camera framing for a scene, derived from the mesh bounds.
 */
struct SceneView
{
  /** @brief World point the camera looks at (mesh bounding-box centre). */
  math::Vec3f center;
  /** @brief World position the camera starts at. */
  math::Vec3f eye;
  /** @brief Camera-relative offset of `eye` from `center` (XZ + height). */
  math::Vec3f eye_offset;
  /** @brief Approx. sphere radius covering the mesh (for framing distance). */
  float radius;
  /** @brief Movement speed chosen from `radius` (see compute_scene_view). */
  float speed;
};

/**
 * @brief Everything knobs the CLI can set for one scene.
 */
struct SceneOptions
{
  /** @brief Path to the OBJ model (required). */
  std::filesystem::path model;
  /** @brief Framebuffer / window width. */
  int width{1280};
  /** @brief Framebuffer / window height. */
  int height{720};
  /** @brief Vertical field of view in degrees. */
  float fov_y{60.0F};
  /** @brief Start in wireframe mode. */
  bool wireframe{false};
  /** @brief Skip texture loading. */
  bool no_textures{false};
  /** @brief Attach a camera-space headlamp light. */
  bool headlamp{false};
  /** @brief Ambient intensity scaling each material's ambient term. */
  float ambient{0.1F};
  /** @brief Place a point (or spot when a direction is given) light here. */
  std::optional<math::Vec3f> light_position;
  /** @brief Aim an extra directional/spot light along this vector. */
  std::optional<math::Vec3f> light_direction;
  /** @brief Colour of the extra light (RGB 0..1). */
  math::Vec3f light_color{1, 1, 1};
  /** @brief Intensity of the extra light. */
  float light_intensity{1.0F};
};

/**
 * @brief A fully loaded, render-ready scene.
 */
struct Scene
{
  SceneOptions         options;
  SceneView            view;
  Mesh                 mesh;
  std::vector<Texture> textures;
  std::vector<LightF>  lights;
};

/**
 * @brief Frame a camera so the whole mesh is comfortably visible.
 * @param mesh   The loaded geometry.
 * @param width,height  Framebuffer size (for aspect).
 * @param fov_y  Vertical field of view (degrees).
 * @return A SceneView with `eye = center + (0, 0.3·radius, fit)` where `fit`
 *         places the model inside the vertical FOV, and movement speed scaled
 *         to `radius`.
 */
[[nodiscard]] auto compute_scene_view(const Mesh& mesh, int width, int height, float fov_y)
  -> SceneView;

/**
 * @brief Apply a SceneView to `camera`.
 * @param camera  Target camera (mutated in place).
 * @param view    Framing from compute_scene_view.
 * @param width,height  Framebuffer size (aspect).
 * @param fov_y   Vertical field of view (degrees).
 * @note Sets the projection, eye position and yaw/pitch orientation.
 */
auto setup_camera(CameraF& camera, const SceneView& view, int width, int height, float fov_y)
  -> void;

/**
 * @brief Build the light list for a scene.
 * @param view     Framing (used to place the default lights).
 * @param options  CLI options; when either `light_position` or
 *                 `light_direction` is set, ONLY the user light(s) are created
 *                 (a directional/spot light from `direction`, a point/spot
 *                 light at `position`), otherwise a default sun + point lamp +
 *                 fill light trio is produced with `radius`-scaled falloff.
 * @return The scene's light list (headlamp is added later by the Viewer).
 */
[[nodiscard]] auto make_lights(const SceneView& view, const SceneOptions& options)
  -> std::vector<LightF>;

/**
 * @brief Load a full scene from options.
 * @param options  Everything required to build the scene.
 * @return A packed Scene (mesh, textures, lights, view) or `std::nullopt` when
 *         the model could not be parsed.
 */
[[nodiscard]] auto load_scene(const SceneOptions& options) -> std::optional<Scene>;

/**
 * @brief Render one frame of a scene and write it to `path`.
 * @param scene  Already-loaded scene.
 * @param path   Destination PPM file.
 * @return True on success.
 * @note Used by snapshot mode; the interactive path goes through
 *       obzcene::Viewer::run instead.
 */
[[nodiscard]] auto render_snapshot(const Scene& scene, const std::string& path) -> bool;

} // namespace obzcene::app