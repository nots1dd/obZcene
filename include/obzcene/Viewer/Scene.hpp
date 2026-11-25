#pragma once

/**
 * @file Scene.hpp
 * @brief The editable scene: a flat list of objects plus lights, plus the
 *        model cache and loader used to populate it.
 *
 * obzcene::Scene is the engine's scene state: everything the interactive
 * viewer or a game mutates lives here (objects with their transforms, the
 * light list). Objects are held by shared_ptr so handles stay valid across
 * edits and removals (the vector only reorders/repoints). Models are shared
 * through a path-keyed cache so adding the same OBJ twice instances it
 * instead of reloading. SceneFile.hpp maps this state to disk; Viewer.hpp
 * and Engine mutate it at runtime.
 *
 * @see Viewer/SceneObject.hpp, Viewer/SceneFile.hpp, src/Viewer/Scene.cpp
 */

#include "obzcene/Math/AABB.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/Mesh.hpp"
#include "obzcene/Renderer/RenderStats.hpp"
#include "obzcene/Renderer/Shader.hpp"
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

/** @brief Shared handle to a placed object; stays valid across scene edits. */
using ObjectHandle = std::shared_ptr<SceneObject>;

/**
 * @brief The scene state: object handles, lights and the model cache.
 */
class Scene
{
public:
  /**
   * @brief All placed objects, in draw order.
   *
   * Held by shared_ptr so a handle obtained from #add_object (or by indexing
   * here) remains valid when other objects are added or removed.
   */
  std::vector<ObjectHandle> objects;
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
   * @brief Append an object for `model` at `position`.
   * @return A stable handle to the new object (also appended to #objects).
   */
  auto add_object(std::shared_ptr<const Model> model, const math::Vec3f& position) -> ObjectHandle;

  /** @brief Remove the object pointed to by `handle` (no-op when not found). */
  auto remove_object(const ObjectHandle& handle) -> void;

  /** @brief Remove the object at `index` (no-op when out of range). */
  auto remove_object_at(usz index) -> void;

  /** @brief Find the index of `handle` in #objects, or std::nullopt. */
  [[nodiscard]] auto index_of(const ObjectHandle& handle) const -> std::optional<usz>;

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
 * @brief Render the whole scene into `fb` (already cleared).
 * @param fb        Target framebuffer.
 * @param scene     The scene to draw.
 * @param camera    Camera for view transform and frustum culling.
 * @param lights    Resolved world-space lights.
 * @param ambient   Global ambient intensity (scales material ambient).
 * @param wireframe Draw outlines instead of shaded triangles.
 * @param filter    Texture filtering for diffuse maps (Nearest keeps
 *                  pixel-art textures crisp; Linear is the default).
 * @return Merged rasterizer counters across all objects.
 *
 * @note Each object is rendered with its model matrix, frustum-culled
 *       against its transform-aware world bounds, with its own texture table.
 *       Objects are drawn nearest-to-camera first so the depth test rejects
 *       occluded fragments before shading (the image itself is unchanged —
 *       depth testing is order-independent).
 */
[[nodiscard]] auto render_scene(FrameBuffer& fb, const Scene& scene, const CameraF& camera,
                                const std::vector<LightF>& lights, float ambient, bool wireframe,
                                TextureFilter filter = TextureFilter::Linear, FogSettings fog = {})
  -> RenderStats;

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
