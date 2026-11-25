/**
 * @file Assets.cpp
 * @brief The asset registry implementation.
 *
 * Name resolution walks the registered roots; loading delegates to the same
 * code paths as the editor (load_model_from_disk, load_audio_clip) so games
 * and the editor see identical assets. Caches are keyed by resolved path so
 * aliases ("skull" vs "models/skull.obj") still share one instance.
 *
 * @see Engine/Assets.hpp (declarations)
 */

#include "obzcene/Engine/Assets.hpp"

#include "obzcene/Core/Logger.hpp"
#include "obzcene/utils/Env-Vars.hpp"

#include <cstdlib>
#include <sstream>
#include <utility>

namespace obzcene
{

namespace
{

/** @brief The active search path as a printable "a; b; c" list. */
[[nodiscard]] auto join_roots(const std::vector<std::filesystem::path>& roots) -> std::string
{
  auto out   = std::string{};
  auto first = true;
  for (const auto& root : roots)
  {
    if (!first)
    {
      out += "; ";
    }
    out += root.string();
    first = false;
  }
  return out;
}

} // namespace

Assets::Assets(std::filesystem::path root, bool no_textures) : m_no_textures(no_textures)
{
  // The environment override comes first — it exists so users can fix a wrong
  // root without recompiling.
  if (const auto* env_root = std::getenv(OBZCENE_ASSETS_DIR_ENV);
      env_root != nullptr && env_root[0] != '\0')
  {
    m_roots.emplace_back(env_root);
    LOG_INFO("Assets: prepending root from {}: {}", OBZCENE_ASSETS_DIR_ENV, env_root);
  }
  if (!root.empty())
  {
    m_roots.push_back(std::move(root));
  }
  m_roots.emplace_back(".");
  LOG_INFO("Assets: search path is [{}]", join_roots(m_roots));
  describe_roots();
}

auto Assets::add_root(const std::filesystem::path& root) -> void
{
  m_roots.push_back(root);
  LOG_INFO("Assets: added root '{}'; search path is now [{}]", root.string(), join_roots(m_roots));
}

auto Assets::root() const -> const std::filesystem::path& { return m_roots.front(); }

auto Assets::roots() const -> const std::vector<std::filesystem::path>& { return m_roots; }

auto Assets::describe_roots() const -> void
{
  for (usz i = 0; i < m_roots.size(); ++i)
  {
    auto       ec      = std::error_code{};
    const auto present = std::filesystem::is_directory(m_roots[i], ec);
    LOG_INFO("Assets:   root[{}] '{}' ({})", i, m_roots[i].string(), present ? "ok" : "MISSING");
  }
}

auto Assets::resolve(const std::string& name, const char* extension) const
  -> std::optional<std::filesystem::path>
{
  const auto with_extension = name.ends_with(extension) ? name : name + extension;
  for (const auto& root : m_roots)
  {
    auto candidate = root / name;
    auto ec        = std::error_code{};
    if (std::filesystem::exists(candidate, ec))
    {
      return candidate;
    }
    candidate = root / with_extension;
    if (std::filesystem::exists(candidate, ec))
    {
      return candidate;
    }
  }
  return std::nullopt;
}

auto Assets::exists(const std::string& name) const -> bool
{
  auto ec = std::error_code{};
  for (const auto& root : m_roots)
  {
    if (std::filesystem::exists(root / name, ec))
    {
      return true;
    }
  }
  return false;
}

auto Assets::model(const std::string& name) -> std::shared_ptr<const Model>
{
  const auto path = resolve(name, ".obj");
  if (!path.has_value())
  {
    LOG_WARN("Model '{}' not found; searched roots: [{}]", name, join_roots(m_roots));
    return nullptr;
  }
  const auto key = path->string();
  if (const auto it = m_models.find(key); it != m_models.end())
  {
    return it->second;
  }
  auto model = load_model_from_disk(*path, m_no_textures);
  if (model != nullptr)
  {
    m_models.emplace(key, model);
  }
  return model;
}

auto Assets::audio(const std::string& name) -> std::shared_ptr<const AudioClip>
{
  const auto path = resolve(name, ".wav");
  if (!path.has_value())
  {
    LOG_WARN("Audio clip '{}' not found; searched roots: [{}]", name, join_roots(m_roots));
    return nullptr;
  }
  const auto key = path->string();
  if (const auto it = m_clips.find(key); it != m_clips.end())
  {
    return it->second;
  }
  auto clip = load_audio_clip(key);
  if (clip != nullptr)
  {
    m_clips.emplace(key, clip);
  }
  return clip;
}

} // namespace obzcene
