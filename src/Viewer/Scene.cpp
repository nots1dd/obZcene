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
#include "obzcene/Texture/TextureLoader.hpp"
#include "obzcene/Viewer/SceneFile.hpp"

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

auto Scene::add_object(std::shared_ptr<const Model> model, const math::Vec3f& position) -> usz
{
  auto object     = SceneObject{};
  object.model    = std::move(model);
  object.position = position;
  objects.push_back(std::move(object));
  return objects.size() - 1;
}

auto Scene::remove_object(usz index) -> void
{
  if (index < objects.size())
  {
    objects.erase(objects.begin() + narrow_cast<std::ptrdiff_t>(index));
  }
}

auto Scene::world_bounds() const -> math::AABBf
{
  auto bounds = math::AABBf::empty();
  for (const auto& object : objects)
  {
    if (object.model == nullptr)
    {
      continue;
    }
    const auto box = object.transformed_bounds(object.model->mesh.bounds);
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
    if (object.model == model)
    {
      ++count;
    }
  }
  return count;
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
    const auto index   = scene.add_object(std::move(model), entry.position);
    auto&      object  = scene.objects[index];
    object.yaw         = entry.yaw;
    object.scale       = entry.scale;
    object.source_path = entry.model_path;
    object.name        = entry.model_path.filename().string();
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
