#pragma once

/**
 * @file SceneFile.hpp
 * @brief Load/save of `.obzscene` text scene files.
 *
 * A scene file is a plain-text, line-oriented description of everything the
 * interactive editor can rebuild: the objects in the scene (OBJ path +
 * transform), the lights, and the camera. Format (one entry per line,
 * whitespace-separated, `#` comments, blank lines ignored):
 *
 *   version 1
 *   object <model.obj> <px> <py> <pz> <yaw> <scale>
 *   light  dir   <dx> <dy> <dz> <r> <g> <b> <intensity>
 *   light  point <px> <py> <pz> <r> <g> <b> <intensity>
 *   light  spot  <px> <py> <pz> <dx> <dy> <dz> <r> <g> <b> <intensity>
 *   camera <px> <py> <pz> <yaw> <pitch>
 *
 * Angles are radians, colours are linear RGB in 0..1. Unknown keywords and
 * malformed lines are skipped with a warning, so newer files degrade
 * gracefully in older builds.
 *
 * @see Viewer/Scene.hpp (the in-memory scene this maps to/from)
 */

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Logger.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Viewer/SceneObject.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace obzcene
{

/** @brief One object entry in a scene file. */
struct SceneFileObject
{
  std::filesystem::path model_path;
  math::Vec3f           position{0.0F};
  float                 yaw{0.0F};
  float                 scale{1.0F};
};

/** @brief A fully parsed scene file. */
struct SceneFileData
{
  std::vector<SceneFileObject> objects;
  std::vector<LightF>          lights;
  /** @brief Camera pose; has_value when the file contained a `camera` line. */
  std::optional<math::Vec3f> camera_position;
  std::optional<float>       camera_yaw;
  std::optional<float>       camera_pitch;
};

namespace scene_file_detail
{

[[nodiscard]] inline auto parse_float(const std::string& token, float& out) -> bool
{
  const auto* first = token.data();
  const auto* last  = first + token.size();
  const auto  res   = std::from_chars(first, last, out);
  return res.ec == std::errc{} && res.ptr == last;
}

inline auto parse_float_token(std::istringstream& iss, float& out) -> bool
{
  std::string token;
  if (!(iss >> token))
  {
    return false;
  }
  return parse_float(token, out);
}

[[nodiscard]] inline auto parse_vec3(std::istringstream& iss, math::Vec3f& out) -> bool
{
  return parse_float_token(iss, out.x) && parse_float_token(iss, out.y) &&
         parse_float_token(iss, out.z);
}

} // namespace scene_file_detail

/**
 * @brief Parse a scene file. Malformed lines are skipped with a warning; a
 *        file that cannot be opened at all yields std::nullopt.
 */
[[nodiscard]] inline auto load_scene_file(const std::filesystem::path& path)
  -> std::optional<SceneFileData>
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    return std::nullopt;
  }

  SceneFileData data;
  std::string   line;
  auto          line_number = 0;
  while (std::getline(file, line))
  {
    ++line_number;
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    std::istringstream iss(line);
    std::string        keyword;
    iss >> keyword;

    const auto warn = [&]()
    { LOG_MODULE_WARN("SCENE", "Skipping malformed line {} in '{}'", line_number, path.string()); };

    if (keyword == "version")
    {
      continue; // single supported version; kept for future-proofing
    }
    if (keyword == "object")
    {
      std::string model;
      float       yaw = 0.0F, scale = 1.0F;
      math::Vec3f position;
      if (!(iss >> model) || !scene_file_detail::parse_vec3(iss, position) ||
          !scene_file_detail::parse_float_token(iss, yaw) ||
          !scene_file_detail::parse_float_token(iss, scale))
      {
        warn();
        continue;
      }
      data.objects.push_back(SceneFileObject{model, position, yaw, scale});
      continue;
    }
    if (keyword == "light")
    {
      std::string type;
      math::Vec3f a, b, color;
      float       intensity = 1.0F;
      if (!(iss >> type) || !scene_file_detail::parse_vec3(iss, a))
      {
        warn();
        continue;
      }
      // Order matches save_scene_file: point: pos; dir: dir; spot: pos dir.
      const bool need_direction = type == "spot";
      if (need_direction && !scene_file_detail::parse_vec3(iss, b))
      {
        warn();
        continue;
      }
      if (!scene_file_detail::parse_vec3(iss, color) ||
          !scene_file_detail::parse_float_token(iss, intensity))
      {
        warn();
        continue;
      }
      if (type == "point")
      {
        data.lights.push_back(LightF::point(a, color, intensity));
      }
      else if (type == "dir")
      {
        data.lights.push_back(LightF::directional(a, color, intensity));
      }
      else if (type == "spot")
      {
        data.lights.push_back(LightF::spot(a, b, color, intensity));
      }
      else
      {
        warn();
      }
      continue;
    }
    if (keyword == "camera")
    {
      math::Vec3f position;
      float       yaw = 0.0F, pitch = 0.0F;
      if (!scene_file_detail::parse_vec3(iss, position) ||
          !scene_file_detail::parse_float_token(iss, yaw) ||
          !scene_file_detail::parse_float_token(iss, pitch))
      {
        warn();
        continue;
      }
      data.camera_position = position;
      data.camera_yaw      = yaw;
      data.camera_pitch    = pitch;
      continue;
    }
    warn();
  }
  return data;
}

/**
 * @brief Write a scene file. Object paths are written as given (relative
 *        paths stay portable when the scene file sits next to the models).
 * @return True when the file was written.
 */
inline auto save_scene_file(const std::filesystem::path&    path,
                            const std::vector<SceneObject>& objects,
                            const std::vector<LightF>& lights, const CameraF& camera) -> bool
{
  std::ofstream file(path);
  if (!file.is_open())
  {
    return false;
  }
  file << "# obZcene scene\nversion 1\n";
  for (const auto& object : objects)
  {
    file << "object " << object.source_path.string() << ' ' << object.position.x << ' '
         << object.position.y << ' ' << object.position.z << ' ' << object.yaw << ' '
         << object.scale << '\n';
  }
  for (const auto& light : lights)
  {
    if (light.binding != LightBinding::WorldSpace)
    {
      continue; // camera-attached lights (headlamp) are runtime state, not scene data
    }
    const auto intensity = light.intensity;
    if (light.type == LightType::Point)
    {
      file << "light point " << light.position.x << ' ' << light.position.y << ' '
           << light.position.z << ' ' << light.color.x << ' ' << light.color.y << ' '
           << light.color.z << ' ' << intensity << '\n';
    }
    else if (light.type == LightType::Directional)
    {
      file << "light dir " << light.direction.x << ' ' << light.direction.y << ' '
           << light.direction.z << ' ' << light.color.x << ' ' << light.color.y << ' '
           << light.color.z << ' ' << intensity << '\n';
    }
    else
    {
      file << "light spot " << light.position.x << ' ' << light.position.y << ' '
           << light.position.z << ' ' << light.direction.x << ' ' << light.direction.y << ' '
           << light.direction.z << ' ' << light.color.x << ' ' << light.color.y << ' '
           << light.color.z << ' ' << intensity << '\n';
    }
  }
  const auto pos = camera.position();
  file << "camera " << pos.x << ' ' << pos.y << ' ' << pos.z << ' ' << camera.yaw() << ' '
       << camera.pitch() << '\n';
  return file.good();
}

} // namespace obzcene
