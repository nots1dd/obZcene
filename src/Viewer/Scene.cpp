/**
 * @file Scene.cpp
 * @brief Model loading and the Scene container implementation.
 *
 * load_model_from_disk is the engine's single OBJ+texture entry point used
 * both by the App's startup scene and by the editor's add-object flow, so the
 * parse/build/texture pipeline is shared and consistent. Scene itself is just
 * the flat object/light lists plus the path-keyed model cache that turns
 * duplicate loads into instances.
 *
 * @see Viewer/Scene.hpp (declarations), Viewer/SceneFile.hpp (persistence)
 */

#include "obzcene/Viewer/Scene.hpp"

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Logger.hpp"
#include "obzcene/File/Obj/Parser/ObjParser.hpp"
#include "obzcene/Renderer/Rasterizer.hpp"
#include "obzcene/Renderer/Shader.hpp"
#include "obzcene/Texture/TextureLoader.hpp"
#include "obzcene/Viewer/SceneFile.hpp"

#include <algorithm>
#include <utility>

namespace obzcene
{

auto load_model_from_disk(const std::filesystem::path& path, bool no_textures)
  -> std::shared_ptr<const Model>
{
  const auto data = parse_obj(path.string());
  if (data.positions.empty())
  {
    LOG_WARN("Failed to load model '{}'", path.string());
    return nullptr;
  }
  auto model           = std::make_shared<Model>();
  model->mesh          = build_mesh(data);
  const auto model_dir = path.has_parent_path() ? path.parent_path() : std::filesystem::path{"."};
  model->textures = no_textures ? std::vector<Texture>{} : load_material_textures(data, model_dir);
  LOG_INFO("Loaded '{}': {} vertices, {} triangles", path.string(), model->mesh.vertices.size(),
           model->mesh.triangles.size());
  return model;
}

auto Scene::load_model(const std::filesystem::path& path, bool no_textures)
  -> std::shared_ptr<const Model>
{
  auto        ec         = std::error_code{};
  const auto  key        = std::filesystem::weakly_canonical(path, ec).string();
  const auto& lookup_key = ec ? path.string() : key;
  if (const auto it = m_model_cache.find(lookup_key); it != m_model_cache.end())
  {
    return it->second;
  }
  auto model = load_model_from_disk(path, no_textures);
  if (model != nullptr)
  {
    m_model_cache.emplace(lookup_key, model);
  }
  return model;
}

auto Scene::add_object(std::shared_ptr<const Model> model, const math::Vec3f& position)
  -> ObjectHandle
{
  auto object      = std::make_shared<SceneObject>();
  object->model    = std::move(model);
  object->position = position;
  objects.push_back(std::move(object));
  return objects.back();
}

auto Scene::remove_object(const ObjectHandle& handle) -> void
{
  const auto it = std::find(objects.begin(), objects.end(), handle);
  if (it != objects.end())
  {
    objects.erase(it);
  }
}

auto Scene::remove_object_at(usz index) -> void
{
  if (index < objects.size())
  {
    objects.erase(objects.begin() + narrow_cast<std::ptrdiff_t>(index));
  }
}

auto Scene::index_of(const ObjectHandle& handle) const -> std::optional<usz>
{
  const auto it = std::find(objects.begin(), objects.end(), handle);
  if (it == objects.end())
  {
    return std::nullopt;
  }
  return static_cast<usz>(std::distance(objects.begin(), it));
}

auto Scene::world_bounds() const -> math::AABBf
{
  auto bounds = math::AABBf::empty();
  for (const auto& object : objects)
  {
    if (object == nullptr || object->model == nullptr)
    {
      continue;
    }
    const auto box = object->transformed_bounds(object->model->mesh.bounds);
    if (box.min.x <= box.max.x)
    {
      bounds.expand(box.min);
      bounds.expand(box.max);
    }
  }
  return bounds;
}

auto Scene::instance_count(const std::shared_ptr<const Model>& model) const -> usz
{
  auto count = usz{0};
  for (const auto& object : objects)
  {
    if (object != nullptr && object->model == model)
    {
      ++count;
    }
  }
  return count;
}

auto render_scene(FrameBuffer& fb, const Scene& scene, const CameraF& camera,
                  const std::vector<LightF>& lights, float ambient, bool wireframe,
                  TextureFilter filter, FogSettings fog) -> RenderStats
{
  // Draw nearest-first: the depth test then rejects occluded fragments before
  // shading them, which slashes overdraw in dense scenes (voxel terrain etc.).
  // Depth testing makes the final image order-independent, so this is a pure
  // performance transform.
  const auto         eye = camera.position();
  std::vector<float> distance_sq(scene.objects.size(), 0.0F);
  std::vector<usz>   order(scene.objects.size());
  for (usz i = 0; i < order.size(); ++i)
  {
    const auto& object = scene.objects[i];
    if (object != nullptr && object->model != nullptr)
    {
      const auto center = object->transformed_bounds(object->model->mesh.bounds).center();
      distance_sq[i]    = (center - eye).length_sq();
    }
    order[i] = i;
  }
  std::sort(order.begin(), order.end(),
            [&](usz lhs, usz rhs) { return distance_sq[lhs] < distance_sq[rhs]; });

  auto stats = RenderStats{};
  for (const auto index : order)
  {
    const auto& object = scene.objects[index];
    if (object == nullptr || object->model == nullptr)
    {
      continue;
    }
    const auto bounds    = object->transformed_bounds(object->model->mesh.bounds);
    const auto transform = ModelTransform{object->model_matrix(), object->normal_matrix()};
    const auto uniforms =
      FragmentUniforms{camera, lights, object->model->textures, ambient, filter, fog};
    merge_stats(stats,
                render_object(fb, object->model->mesh, uniforms, wireframe, transform, &bounds));
  }
  return stats;
}

auto apply_scene_file(Scene& scene, const SceneFileData& data,
                      const std::filesystem::path& scene_dir, bool no_textures) -> void
{
  scene.objects.clear();
  scene.lights = data.lights;
  for (const auto& entry : data.objects)
  {
    const auto resolved =
      entry.model_path.is_absolute() ? entry.model_path : scene_dir / entry.model_path;
    auto model = scene.load_model(resolved, no_textures);
    if (model == nullptr)
    {
      LOG_WARN("Scene file entry '{}' failed to load; skipped", resolved.string());
      continue;
    }
    auto object         = scene.add_object(std::move(model), entry.position);
    object->rotation    = entry.rotation;
    object->scale       = entry.scale;
    object->source_path = entry.model_path;
    object->name        = entry.model_path.filename().string();
  }
  LOG_INFO("Scene file applied: {} object(s), {} light(s)", scene.objects.size(),
           scene.lights.size());
}

auto save_scene(const Scene& scene, const std::filesystem::path& path, const CameraF& camera)
  -> bool
{
  return save_scene_file(path, scene.objects, scene.lights, camera);
}

} // namespace obzcene
