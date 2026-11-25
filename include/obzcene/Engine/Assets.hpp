#pragma once

/**
 * @file Assets.hpp
 * @brief Named asset registry for models and audio (namespace obzcene).
 *
 * Games reference assets by short names ("skull", "laser") rather than paths,
 * and the registry resolves them against one or more root directories,
 * loading through the same code paths as the editor (Scene::load_model,
 * load_audio_clip). Assets are shared via shared_ptr, so spawning the same
 * model twice instances it.
 *
 * @see src/Engine/Assets.cpp (implementation), Viewer/Scene.hpp (models),
 *      Engine/Audio.hpp (clips)
 */

#include "obzcene/Engine/Audio.hpp"
#include "obzcene/Viewer/Scene.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace obzcene
{

/**
 * @brief The asset registry: named models and audio clips.
 *
 * Names are looked up in the registered root directories (in the order they
 * were added). A loaded asset is cached by its resolved path; asking for the
 * same name twice returns the same shared instance.
 */
class Assets
{
public:
  /**
   * @brief A registry with `root` as the first search directory.
   * @param no_textures  Skip texture loading for models (headless runs).
   *
   * The active search path is logged at construction; when the
   * `OBZCENE_ASSETS_DIR` environment variable is set, that directory is
   * searched first (before `root` and the working-directory fallback), which
   * makes misconfigured asset paths easy to diagnose without recompiling.
   */
  explicit Assets(std::filesystem::path root = {}, bool no_textures = false);

  /** @brief Add another directory to search (later entries are fallbacks). */
  auto add_root(const std::filesystem::path& root) -> void;
  /** @brief The primary search directory. */
  [[nodiscard]] auto root() const -> const std::filesystem::path&;
  /**
   * @brief All directories searched, in priority order.
   *
   * The list always ends with the working-directory fallback; when
   * `OBZCENE_ASSETS_DIR` is set it comes first.
   */
  [[nodiscard]] auto roots() const -> const std::vector<std::filesystem::path>&;
  /**
   * @brief Print the active search path and per-root status to the log.
   *
   * Call this when an asset fails to load: it lists every root, whether it
   * exists on disk, and which environment/config knob produced it.
   */
  auto describe_roots() const -> void;

  /**
   * @brief Load (or return the cached) model registered under `name`.
   * @param name  Relative path or bare name ("skull.obj", "skull"); bare
   *              names are searched in the roots with and without ".obj".
   * @return The model, or nullptr when it could not be found/parsed.
   */
  [[nodiscard]] auto model(const std::string& name) -> std::shared_ptr<const Model>;

  /**
   * @brief Load (or return the cached) audio clip registered under `name`.
   * @param name  Relative path or bare name ("laser.wav", "laser").
   * @return The clip, or nullptr when it could not be found/decoded.
   */
  [[nodiscard]] auto audio(const std::string& name) -> std::shared_ptr<const AudioClip>;

  /** @brief True when `name` resolved to a file in a registered root. */
  [[nodiscard]] auto exists(const std::string& name) const -> bool;

private:
  /** @brief Resolve `name` against the roots to an existing file. */
  [[nodiscard]] auto resolve(const std::string& name, const char* extension) const
    -> std::optional<std::filesystem::path>;

  std::vector<std::filesystem::path> m_roots;
  bool                               m_no_textures{false};
  /** @brief Model cache keyed by resolved path. */
  std::unordered_map<std::string, std::shared_ptr<const Model>> m_models;
  /** @brief Clip cache keyed by resolved path. */
  std::unordered_map<std::string, std::shared_ptr<const AudioClip>> m_clips;
};

} // namespace obzcene
