#pragma once

/**
 * @file GpuRenderer.hpp
 * @brief OpenGL 3.3 core backend: the GPU counterpart of the CPU rasterizer.
 *
 * GpuRenderer draws an obzcene::Scene through the same conventions as the
 * software path — Camera::view_projection (right-handed, NDC z in [-1,1]),
 * the Material/Light fields from Core/Types.hpp + Light.hpp, and the
 * material<->texture index alignment — so a game can flip between
 * RendererBackend::Cpu and RendererBackend::Gpu without touching its scene.
 *
 * Meshes and diffuse textures are uploaded lazily and cached by source
 * pointer (meshes) / content hash (textures), so static scenes pay upload
 * once. A CPU-side overlay layer (the framebuffer games draw in
 * Game::on_render, plus the HUD) is composited as an alpha-blended
 * fullscreen quad, keeping 2D game code backend-agnostic.
 *
 * @see src/Renderer/GpuRenderer.cpp (GL loading + shaders),
 *      Renderer/Rasterizer.hpp (the CPU path), Engine/Engine.hpp
 *      (EngineConfig::backend)
 */

#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Color.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Light.hpp"
#include "obzcene/Renderer/RenderStats.hpp"
#include "obzcene/Renderer/Shader.hpp"
#include "obzcene/Texture/Texture.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace obzcene
{
class Scene;

/** @brief Which renderer the engine runs (see EngineConfig::backend). */
enum class RendererBackend
{
  /** @brief Software rasterizer into a CPU framebuffer (default). */
  Cpu,
  /** @brief OpenGL 3.3 core (hardware), with a CPU overlay layer. */
  Gpu,
};

/**
 * @brief The GPU scene renderer. One instance per window/GL context.
 *
 * All methods must be called with the GL context current (Engine owns this
 * when EngineConfig::backend == RendererBackend::Gpu).
 */
class GpuRenderer
{
public:
  GpuRenderer();  ///< Defined out-of-line (Impl is incomplete here).
  ~GpuRenderer(); ///< Releases GL objects (requires a current GL context).

  GpuRenderer(const GpuRenderer&)                    = delete;
  auto operator=(const GpuRenderer&) -> GpuRenderer& = delete;

  /**
   * @brief Load GL entry points and compile the shader programs.
   * @return False (renderer unusable) when the context or shaders fail.
   */
  auto init() -> bool;
  /** @brief True after a successful #init. */
  [[nodiscard]] auto ok() const -> bool;
  /** @brief Last init failure reason (shader log etc.). */
  [[nodiscard]] auto last_error() const -> const std::string&;

  /**
   * @brief Draw the scene (colour + depth cleared first).
   * @param scene,camera,lights,ambient,wireframe  Same meaning as
   *        obzcene::render_scene (lights already resolved).
   * @param clear   Sky/background colour.
   * @param filter  Diffuse-map filtering (Nearest keeps pixel art crisp).
   * @param width,height  Drawable size in pixels (GL viewport).
   * @return Empty RenderStats (the CPU counters do not exist on the GPU).
   */
  auto render(const Scene& scene, const CameraF& camera, const std::vector<LightF>& lights,
              float ambient, bool wireframe, const Color& clear, TextureFilter filter,
              const FogSettings& fog, int width, int height, const Color* sky_top = nullptr,
              const Color* sky_bottom = nullptr) -> RenderStats;

  /**
   * @brief Alpha-blend `overlay` over the rendered frame (fullscreen quad).
   * @note The CPU overlay framebuffer treats {0,0,0,0} as transparent; Engine
   *       clears it that way in GPU mode before Game::on_render.
   */
  auto draw_overlay(const FrameBuffer& overlay) -> void;

  /**
   * @brief Read the back buffer into `out` (before the swap; for tests and
   *        snapshots). Slow — stalls the pipeline.
   */
  auto readback(FrameBuffer& out) -> bool;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace obzcene
