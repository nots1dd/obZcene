#pragma once

/**
 * @file Scene.hpp
 * @brief The editable scene: a flat list of objects plus lights, plus the
 *        model cache and loader used to populate it.
 *
 * obzcene::Scene is the editor's document model: everything the interactive
 * viewer mutates lives here (objects with their transforms, the light list).
 * Models are shared through a path-keyed cache so adding the same OBJ twice
 * instances it instead of reloading. SceneFile.hpp maps this state to disk;
 * Viewer.hpp mutates it at runtime.
 *
 * @see Viewer/SceneObject.hpp, Viewer/SceneFile.hpp, src/Viewer/Scene.cpp
 */

#include "obzcene/Math/AABB.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/Mesh.hpp"
#include "obzcene/Texture/Texture.hpp"
#include "obzcene/Viewer/SceneObject.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace obzcene
{

/**
 * @brief A loaded model: geometry plus its diffuse texture table.
 *
 * The texture vector is index-aligned with `Model::mesh.materials`, which is
 * how FragmentUniforms matches fragments to maps (see TextureLoader.hpp).
 */
struct Model
{
  Mesh                 mesh;
  std::vector<Texture> textures;
};

/**
 * @brief The editor's scene state: objects, lights and the model cache.
 */
class Scene
{
public:
  /** @brief All placed objects, in draw order. */
  std::vector<SceneObject> objects;
  /** @brief Scene lights (world-space; the headlamp is a viewer-side extra). */
  std::vector<LightF> lights;

  /**
   * @brief Load a model from disk, returning the cached instance when the
   *        path was loaded before.
   * @param path         The `.obj` file.
   * @param no_textures  Skip texture loading.
   * @return The model, or nullptr when the file could not be parsed.
   */
  [[nodiscard]] auto load_model(const std::filesystem::path& path, bool no_textures = false)
    -> std::shared_ptr<const Model>;

  /**
   * @brief Append an object for `model` at `position` and select it.
   * @return Index into #objects of the new object.
   */
  auto add_object(std::shared_ptr<const Model> model, const math::Vec3f& position) -> usz;

  /** @brief Remove object `index` (no-op when out of range). */
  auto remove_object(usz index) -> void;

  /**
   * @brief Combined world-space bounds of all objects (transform-aware).
   * @return An empty AABB when the scene has no objects.
   */
  [[nodiscard]] auto world_bounds() const -> math::AABBf;

  /**
   * @brief Number of live instances of `model` (used by tests; cheap sanity
   *        check that the cache actually shares).
   */
  [[nodiscard]] auto instance_count(const std::shared_ptr<const Model>& model) const -> usz;

private:
  /** @brief Loaded models keyed by canonical path string. */
  std::unordered_map<std::string, std::shared_ptr<const Model>> m_model_cache;
};

/**
 * @brief Load a model: parse the OBJ, build the mesh, load its textures.
 * @param path         The `.obj` file; textures resolve relative to its parent.
 * @param no_textures  Skip texture loading (for --no-textures).
 * @return The model, or nullptr when parsing produced no geometry.
 */
[[nodiscard]] auto load_model_from_disk(const std::filesystem::path& path, bool no_textures)
  -> std::shared_ptr<const Model>;

struct SceneFileData;

/**
 * @brief Rebuild a scene from parsed scene-file data.
 * @param data         Output of obzcene::load_scene_file.
 * @param scene_dir    Directory the scene file lives in; relative object
 *                     paths resolve against it.
 * @param no_textures  Skip texture loading.
 * @return The populated scene; object entries whose models fail to load are
 *         skipped with a warning (the scene still opens).
 */
auto apply_scene_file(Scene& scene, const SceneFileData& data,
                      const std::filesystem::path& scene_dir, bool no_textures) -> void;

/**
 * @brief Serialize the scene to a file.
 * @param path  Destination (parent directory must exist).
 * @return True on success.
 */
[[nodiscard]] auto save_scene(const Scene& scene, const std::filesystem::path& path,
                              const CameraF& camera) -> bool;

} // namespace obzcene
