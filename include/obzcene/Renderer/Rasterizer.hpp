#pragma once

/**
 * @file Rasterizer.hpp
 * @brief Software triangle rasterizer: transform, clip, cull, interpolate, shade.
 *
 * `rasterize` is the hot loop of the renderer and runs in two parallel
 * phases (threads fanned out via obzcene::threading::parallel_run, count
 * = RasterOptions::thread_count or hardware_concurrency):
 *
 * Phase 1: each worker transforms a contiguous chunk of the vertex pool
 * (world -> clip -> screen), then — after a barrier — classifies a chunk of
 * the triangle list: near/far clip-w test, degenerate/backface/viewport
 * culling, and emission of compact RasterJobs binned by the framebuffer band
 * they overlap. The rare triangles straddling the near/far planes are not
 * culled but deferred to a short serial clip pass (Sutherland-Hodgman in
 * clip space, fan-triangulated), so geometry crossing the near plane no
 * longer pops out.
 *
 * Phase 2: each worker owns a horizontal band of pixel rows and
 * scan-converts only the jobs binned into it. Bands partition rows
 * disjointly, so colour/depth writes never race; jobs are visited in
 * triangle order within a band, so results are identical to a single-band
 * run. The inner loop evaluates coverage and depth four pixels at a time
 * through math::Pack4f and shades only pixels passing the depth test.
 *
 * The `raster_detail` helpers (RasterVertex / perspective_blend) mirror the
 * classic perspective-correct barycentric approach and are exercised directly
 * by tests/test_culling.cpp. Thread fan-out lives in
 * obzcene::threading::parallel_run (Core/Threading.hpp); the 4-wide pixel
 * arithmetic uses obzcene::math::Pack4f (Math/Pack4.hpp) so the scanline code
 * has a single code path with an SSE2 backend when available.
 *
 * @see Renderer/Shader.hpp, Renderer/FrameBuffer.hpp, Renderer/Mesh.hpp,
 *      Core/Threading.hpp, Math/Pack4.hpp
 */

#include "obzcene/Core/Casts.hpp"
#include "obzcene/Core/Threading.hpp"
#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Mat3.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Math/Pack4.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Math/Vec4.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Color.hpp"
#include "obzcene/Renderer/FrameBuffer.hpp"
#include "obzcene/Renderer/Mesh.hpp"
#include "obzcene/Renderer/RenderStats.hpp"
#include "obzcene/Renderer/Shader.hpp"

#include <algorithm>
#include <array>
#include <barrier>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace obzcene
{

/** @brief Frame-wide switches influencing rasterization. */
struct RasterOptions
{
  /** @brief Reject triangles whose screen-space winding is clockwise (CCW front). */
  bool backface_cull{false};
  /** @brief Worker count for both phases; 0 = hardware concurrency. */
  ui32 thread_count{0};
};

/** @brief Default background colour shared by the viewer and snapshot paths. */
inline constexpr Color kClearColor{22, 22, 28};
/** @brief Default wireframe line colour shared by the viewer and snapshot paths. */
inline constexpr Color kWireframeColor{220, 220, 230};

/**
 * @brief Model-space -> world-space transform for one rendered object.
 *
 * `matrix` maps vertex positions to world space; `normal` is the
 * inverse-transpose of its 3x3 block (rotations and uniform scales are
 * exactly orthogonal-up-to-scale, so a rotation matrix suffices there).
 * Identity by default, in which case rendering is exactly the pre-transform
 * behaviour.
 */
struct ModelTransform
{
  math::Mat4f matrix{math::Mat4f::identity()};
  math::Mat3f normal{math::Mat3f::identity()};
};

/**
 * @brief Internal vertex bookkeeping, clipping and perspective-correct
 *        blending helpers. RasterVertex is kept for the projection unit
 *        tests; PreparedVertex is what the production path uses.
 */
namespace raster_detail
{

/**
 * @brief A vertex after the world-to-screen transform (NOT depth corrected).
 *
 * Used by the tests to reason about projection; the production path uses
 * `PreparedVertex`. `ndc_z` is the interpolated depth, `inv_w` the inverse
 * clip w used for perspective-correct interpolation.
 */
struct RasterVertex
{
  math::Vec2f screen;
  math::Vec3f world;
  math::Vec2f uv;
  math::Vec3f normal;
  float       ndc_z;
  float       inv_w;
};

/**
 * @brief A vertex with NDC/clip data cached for fast rasterization.
 *
 * `screen` holds pixel coordinates, `clip_w`/`inv_w` drive the per-pixel
 * perspective-correct interpolation weights, `ndc_z` is the depth test value.
 * The full clip position is deliberately NOT cached: only the rare
 * near/far-straddling triangles need it, and they recompute it from `world`.
 */
struct PreparedVertex
{
  math::Vec2f screen;
  math::Vec3f world;
  math::Vec2f uv;
  math::Vec3f normal;
  float       clip_w;
  float       inv_w;
  float       ndc_z;
};

/**
 * @brief A clip-space vertex with attributes, used only while clipping.
 */
struct ClipVertex
{
  math::Vec4f clip;
  math::Vec3f world;
  math::Vec3f normal;
  math::Vec2f uv;
};

/**
 * @brief Fill the derived fields of `v` (screen, clip_w, inv_w, ndc_z) from a
 *        clip-space position.
 */
inline auto project_vertex(PreparedVertex& v, const math::Vec4f& clip, int fb_width, int fb_height)
  -> void
{
  v.clip_w         = clip.w;
  v.inv_w          = 1.0F / clip.w;
  const auto ndc_x = clip.x * v.inv_w;
  const auto ndc_y = clip.y * v.inv_w;
  v.screen.x       = (ndc_x * 0.5F + 0.5F) * static_cast<float>(fb_width);
  v.screen.y       = (0.5F - ndc_y * 0.5F) * static_cast<float>(fb_height);
  v.ndc_z          = clip.z * v.inv_w;
}

/**
 * @brief Interpolate two clip vertices along their clip-space edge.
 *
 * Clip space is linear in the world-space edge parameter, so lerping every
 * attribute with the same `t` lands exactly on the intersection point, and
 * later perspective-correct interpolation stays consistent.
 */
inline auto lerp_clip(const ClipVertex& a, const ClipVertex& b, float t) -> ClipVertex
{
  return ClipVertex{a.clip + (b.clip - a.clip) * t, a.world + (b.world - a.world) * t,
                    a.normal + (b.normal - a.normal) * t, a.uv + (b.uv - a.uv) * t};
}

/**
 * @brief Clip a polygon against a clip-w plane (Sutherland-Hodgman).
 * @param in,in_count  Input polygon (3+ vertices).
 * @param plane_w      Plane position: the near or far clip distance.
 * @param keep_above   True keeps the `w > plane_w` side (near plane), false
 *                     keeps `w < plane_w` (far plane).
 * @param out          Output polygon; must hold at least `in_count + 1`.
 * @return Output vertex count (0 when fully clipped away).
 */
inline auto clip_polygon_w(const ClipVertex* in, usz in_count, float plane_w, bool keep_above,
                           ClipVertex* out) -> usz
{
  usz        out_count = 0;
  const auto inside    = [plane_w, keep_above](float w)
  { return keep_above ? w > plane_w : w < plane_w; };
  for (usz i = 0; i < in_count; ++i)
  {
    const auto& curr    = in[i];
    const auto& prev    = in[(i + in_count - 1) % in_count];
    const bool  curr_in = inside(curr.clip.w);
    const bool  prev_in = inside(prev.clip.w);
    if (curr_in != prev_in)
    {
      const auto t     = (plane_w - prev.clip.w) / (curr.clip.w - prev.clip.w);
      out[out_count++] = lerp_clip(prev, curr, t);
    }
    if (curr_in)
    {
      out[out_count++] = curr;
    }
  }
  return out_count;
}

/**
 * @brief Perspective-correct blend of three interpolants using clip w.
 * @param wa,wb,wc      Barycentric weights (sum to 1).
 * @param inv_wa,inv_wb,inv_wc  Per-vertex 1/clip.w.
 * @param a,b,c         The three corner values to interpolate.
 * @return `1/(Σw·inv_w) · Σ w·inv_w·corner`, which linearizes the attribute
 *         in world space instead of screen space.
 */
template <typename T>
[[nodiscard]] auto perspective_blend(float wa, float wb, float wc, float inv_wa, float inv_wb,
                                     float inv_wc, const T& a, const T& b, const T& c) -> T
{
  const auto denom = wa * inv_wa + wb * inv_wb + wc * inv_wc;
  return (a * (wa * inv_wa) + b * (wb * inv_wb) + c * (wc * inv_wc)) / denom;
}

/**
 * @brief One triangle ready for scan conversion: vertex pool indices plus
 *        precomputed screen-space edge data.
 */
struct RasterJob
{
  ui32        a, b, c;
  ui32        material_index;
  int         x_min, y_min, x_max, y_max;
  float       inv_denom;
  math::Vec2f edge_a; ///< c.screen - b.screen
  math::Vec2f edge_b; ///< a.screen - c.screen
};

/** @brief Per-chunk output of the parallel triangle setup. */
struct SetupChunk
{
  /** @brief Triangle-level counters for this chunk (summed by the caller). */
  RenderStats stats;
  /** @brief Surviving triangles in this chunk, in original order. */
  std::vector<RasterJob> jobs;
  /** @brief bins[b] = indices into `jobs` whose bbox overlaps band `b`. */
  std::vector<std::vector<ui32>> bins;
  /** @brief Indices of triangles straddling the near/far planes (clipped serially later). */
  std::vector<ui32> straddlers;
};

/** @brief How classify_triangle() rejected (or accepted) a triangle. */
enum class TriCull
{
  Render,
  Degenerate,
  Backface,
  Viewport,
};

/**
 * @brief Screen-space triangle classification: area, winding, viewport bbox.
 * @param out  On Render, receives the precomputed edge data for scan conversion.
 */
inline auto classify_triangle(const PreparedVertex& a, const PreparedVertex& b,
                              const PreparedVertex& c, bool backface_cull, int fb_width,
                              int fb_height, RasterJob& out) -> TriCull
{
  const auto denom = math::cross(b.screen - a.screen, c.screen - a.screen);
  if (std::abs(denom) < 1.0e-6F)
  {
    return TriCull::Degenerate;
  }
  if (backface_cull && denom >= 0.0F)
  {
    return TriCull::Backface;
  }
  const auto x_min =
    std::max(static_cast<int>(std::min(a.screen.x, std::min(b.screen.x, c.screen.x))), 0);
  const auto x_max = std::min(
    static_cast<int>(std::max(a.screen.x, std::max(b.screen.x, c.screen.x))), fb_width - 1);
  const auto y_min =
    std::max(static_cast<int>(std::min(a.screen.y, std::min(b.screen.y, c.screen.y))), 0);
  const auto y_max = std::min(
    static_cast<int>(std::max(a.screen.y, std::max(b.screen.y, c.screen.y))), fb_height - 1);
  if (x_min > x_max || y_min > y_max)
  {
    return TriCull::Viewport;
  }
  out.x_min     = x_min;
  out.y_min     = y_min;
  out.x_max     = x_max;
  out.y_max     = y_max;
  out.inv_denom = 1.0F / denom;
  out.edge_a    = c.screen - b.screen;
  out.edge_b    = a.screen - c.screen;
  return TriCull::Render;
}

/**
 * @brief Append a job to the chunk and bin it into every band it overlaps.
 */
inline auto emit_job(SetupChunk& chunk, const RasterJob& job, int band_count, int rows_per_band)
  -> void
{
  const auto job_index = narrow_cast<ui32>(chunk.jobs.size());
  chunk.jobs.push_back(job);
  auto band_lo = job.y_min / rows_per_band;
  auto band_hi = job.y_max / rows_per_band;
  band_lo      = std::clamp(band_lo, 0, band_count - 1);
  band_hi      = std::clamp(band_hi, 0, band_count - 1);
  for (auto band = band_lo; band <= band_hi; ++band)
  {
    chunk.bins[static_cast<usz>(band)].push_back(job_index);
  }
}

/** @brief popcount of a 4-bit coverage mask (portable, no intrinsics needed). */
inline constexpr unsigned char kPopcount4[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

/**
 * @brief Malloc-backed scratch storage for the prepared-vertex pool.
 *
 * Unlike `std::vector::resize`, growing this buffer does not value-initialize
 * the elements — the transform phase overwrites every field anyway, so a
 * vector would pay a full extra memset over the pool each frame (hundreds of
 * MB for large meshes). PreparedVertex is an aggregate of trivially copyable
 * members, so realloc-style growth is safe.
 */
struct PoolStorage
{
  PreparedVertex* data{nullptr};
  usz             capacity{0};

  PoolStorage()                                      = default;
  PoolStorage(const PoolStorage&)                    = delete;
  auto operator=(const PoolStorage&) -> PoolStorage& = delete;
  ~PoolStorage() { std::free(data); }

  /** @brief Ensure room for `needed` vertices; shrink when vastly oversized. */
  auto reserve(usz needed) -> void
  {
    if (needed <= capacity)
    {
      return;
    }
    auto new_capacity = std::max(needed, capacity * 2);
    data = static_cast<PreparedVertex*>(std::realloc(data, new_capacity * sizeof(PreparedVertex)));
    capacity = new_capacity;
  }

  /** @brief Release memory when the live set shrank far below capacity. */
  auto shrink_if_huge(usz live) -> void
  {
    if (capacity > live * 4 && capacity > (usz{1} << 12))
    {
      data     = static_cast<PreparedVertex*>(std::realloc(data, live * sizeof(PreparedVertex)));
      capacity = live;
    }
  }
};

/**
 * @brief Scan-convert one prepared triangle over the rows [y_lo, y_hi].
 *
 * The inner loop evaluates coverage and depth four pixels at a time through
 * math::Pack4f, replicating the scalar math lane-for-lane (same operation
 * order), so per-pixel values are identical on the SSE2 and scalar backends;
 * shading runs scalar per surviving pixel. Only pixel counters are touched.
 */
inline auto rasterize_triangle_scan(const PreparedVertex& a, const PreparedVertex& b,
                                    const PreparedVertex& c, ui32 material_index,
                                    const Material& material, const FragmentUniforms& uniforms,
                                    FrameBuffer& fb, int x_min, int x_max, int y_lo, int y_hi,
                                    float inv_denom, const math::Vec2f& edge_a,
                                    const math::Vec2f& edge_b, RenderStats& stats) -> void
{
  const auto     fb_width  = fb.width();
  const auto     width_u   = static_cast<usz>(fb_width);
  constexpr auto row_scale = 0.5F;
  constexpr auto cov_eps   = -1.0e-4F;

  const auto d_wa_dx = -edge_a.y * inv_denom;
  const auto d_wb_dx = -edge_b.y * inv_denom;
  const auto d_wc_dx = -(d_wa_dx + d_wb_dx);

  const auto row_x = static_cast<float>(x_min) + row_scale;
  for (auto py = y_lo; py <= y_hi; ++py)
  {
    const auto row_point = math::Vec2f{row_x, static_cast<float>(py) + row_scale};
    auto       wa        = math::cross(edge_a, row_point - b.screen) * inv_denom;
    auto       wb        = math::cross(edge_b, row_point - c.screen) * inv_denom;
    auto       wc        = 1.0F - wa - wb;
    auto       pixel     = width_u * static_cast<usz>(py) + static_cast<usz>(x_min);
    auto       px        = x_min;

    if (px + 3 <= x_max)
    {
      const math::Pack4f eps4{cov_eps};
      const math::Pack4f one4{1.0F};
      const math::Pack4f ia4{a.inv_w};
      const math::Pack4f ib4{b.inv_w};
      const math::Pack4f ic4{c.inv_w};
      const math::Pack4f za4{a.ndc_z};
      const math::Pack4f zb4{b.ndc_z};
      const math::Pack4f zc4{c.ndc_z};

      const auto dwa4x4 = math::Pack4f{4.0F * d_wa_dx};
      const auto dwb4x4 = math::Pack4f{4.0F * d_wb_dx};
      const auto dwc4x4 = math::Pack4f{4.0F * d_wc_dx};

      auto wa4 = math::Pack4f{wa} + math::Pack4f::iota() * math::Pack4f{d_wa_dx};
      auto wb4 = math::Pack4f{wb} + math::Pack4f::iota() * math::Pack4f{d_wb_dx};
      auto wc4 = math::Pack4f{wc} + math::Pack4f::iota() * math::Pack4f{d_wc_dx};

      for (; px + 3 <= x_max; px += 4)
      {
        // Lane math replicates the scalar loop exactly (same op order).
        const auto cov      = (wa4 >= eps4) & (wb4 >= eps4) & (wc4 >= eps4);
        const auto cov_mask = cov.movemask();
        stats.pixels_shaded += kPopcount4[cov_mask];
        if (cov_mask != 0)
        {
          const auto msa       = wa4 * ia4;
          const auto msb       = wb4 * ib4;
          const auto msc       = wc4 * ic4;
          const auto sum4      = (msa + msb) + msc;
          const auto inv_sum4  = one4 / sum4;
          const auto na4       = msa * inv_sum4;
          const auto nb4       = msb * inv_sum4;
          const auto nc4       = msc * inv_sum4;
          const auto depth4    = (na4 * za4 + nb4 * zb4) + nc4 * zc4;
          const auto buf4      = math::Pack4f::load(&fb.depth_at(pixel));
          const auto pass      = cov & (depth4 < buf4);
          const auto pass_mask = pass.movemask();
          if (pass_mask != 0)
          {
            alignas(16) float na_l[4], nb_l[4], nc_l[4], depth_l[4];
            na4.store(na_l);
            nb4.store(nb_l);
            nc4.store(nc_l);
            depth4.store(depth_l);
            for (auto lane = 0; lane < 4; ++lane)
            {
              if ((pass_mask & (1 << lane)) == 0)
              {
                continue;
              }
              const auto    na = na_l[lane];
              const auto    nb = nb_l[lane];
              const auto    nc = nc_l[lane];
              FragmentInput input;
              input.world_position = na * a.world + nb * b.world + nc * c.world;
              input.normal         = na * a.normal + nb * b.normal + nc * c.normal;
              input.uv             = na * a.uv + nb * b.uv + nc * c.uv;
              input.material_index = material_index;
              fb.color_at(pixel + static_cast<usz>(lane)) =
                shade_fragment(input, material, uniforms);
              fb.depth_at(pixel + static_cast<usz>(lane)) = depth_l[lane];
              ++stats.pixels_written;
            }
          }
        }
        wa4 = wa4 + dwa4x4;
        wb4 = wb4 + dwb4x4;
        wc4 = wc4 + dwc4x4;
        pixel += 4;
      }
      // Hand the current weights to the scalar tail (lane 0 == px).
      alignas(16) float cur[4];
      wa4.store(cur);
      wa = cur[0];
      wb4.store(cur);
      wb = cur[0];
      wc4.store(cur);
      wc = cur[0];
    }

    for (; px <= x_max; ++px)
    {
      if (wa >= cov_eps && wb >= cov_eps && wc >= cov_eps)
      {
        ++stats.pixels_shaded;
        const auto sum     = wa * a.inv_w + wb * b.inv_w + wc * c.inv_w;
        const auto inv_sum = 1.0F / sum;
        const auto na      = wa * a.inv_w * inv_sum;
        const auto nb      = wb * b.inv_w * inv_sum;
        const auto nc      = wc * c.inv_w * inv_sum;
        const auto depth   = na * a.ndc_z + nb * b.ndc_z + nc * c.ndc_z;
        if (depth < fb.depth_at(pixel))
        {
          FragmentInput input;
          input.world_position = na * a.world + nb * b.world + nc * c.world;
          input.normal         = na * a.normal + nb * b.normal + nc * c.normal;
          input.uv             = na * a.uv + nb * b.uv + nc * c.uv;
          input.material_index = material_index;
          fb.color_at(pixel)   = shade_fragment(input, material, uniforms);
          fb.depth_at(pixel)   = depth;
          ++stats.pixels_written;
        }
      }
      wa += d_wa_dx;
      wb += d_wb_dx;
      wc += d_wc_dx;
      ++pixel;
    }
  }
}

/**
 * @brief Scan-convert the rows [y_begin, y_end) of one chunk's binned jobs.
 * @return Pixel-level counters only (triangle counters live in SetupChunk).
 */
inline auto rasterize_band(const PreparedVertex* pool, const std::vector<RasterJob>& jobs,
                           const std::vector<ui32>& bin, const Mesh& mesh,
                           const FragmentUniforms& uniforms, FrameBuffer& fb, int y_begin,
                           int y_end) -> RenderStats
{
  RenderStats stats;
  for (const auto job_index : bin)
  {
    const auto& job  = jobs[job_index];
    const auto  y_lo = std::max(job.y_min, y_begin);
    const auto  y_hi = std::min(job.y_max, y_end - 1);
    if (y_lo > y_hi || job.x_min > job.x_max)
    {
      continue;
    }

    const auto material_index =
      job.material_index < mesh.materials.size() ? job.material_index : 0u;
    const auto& material = mesh.materials[material_index];

    rasterize_triangle_scan(pool[job.a], pool[job.b], pool[job.c], job.material_index, material,
                            uniforms, fb, job.x_min, job.x_max, y_lo, y_hi, job.inv_denom,
                            job.edge_a, job.edge_b, stats);
  }
  return stats;
}

} // namespace raster_detail

/**
 * @brief Bresenham line-drawing primitive between two pixels.
 * @param x0,y0,x1,y1  Endpoints (clipped per-pixel against the framebuffer).
 * @param color        Colour written to every covered pixel.
 * @note Used by #rasterize_wireframe.
 */
inline auto draw_line(FrameBuffer& fb, int x0, int y0, int x1, int y1, const Color& color) -> void
{
  auto       dx  = std::abs(x1 - x0);
  auto       dy  = -std::abs(y1 - y0);
  const auto sx  = x0 < x1 ? 1 : -1;
  const auto sy  = y0 < y1 ? 1 : -1;
  auto       err = dx + dy;
  while (true)
  {
    if (fb.inside(x0, y0))
    {
      fb.at(x0, y0) = color;
    }
    if (x0 == x1 && y0 == y1)
    {
      break;
    }
    const auto e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

/**
 * @brief Project `world` through the camera into framebuffer pixels.
 * @note The inverse-w division can explode for points behind the near plane;
 *       used only by the wireframe debug path (no culling).
 */
inline auto rasterize_wireframe(const Mesh& mesh, const FragmentUniforms& uniforms, FrameBuffer& fb,
                                const Color&       color,
                                const math::Mat4f& model = math::Mat4f::identity()) -> void
{
  const auto vp        = uniforms.camera.view_projection();
  auto       to_screen = [&](const math::Vec3f& position) -> math::Vec2i
  {
    const auto clip = vp * (model * math::Vec4f{position.x, position.y, position.z, 1.0F});
    const auto ndc  = math::Vec2f{clip.x / clip.w, clip.y / clip.w};
    return math::Vec2i{static_cast<int>((ndc.x * 0.5F + 0.5F) * fb.width()),
                       static_cast<int>((0.5F - ndc.y * 0.5F) * fb.height())};
  };
  for (const auto& tri : mesh.triangles)
  {
    const auto a = to_screen(mesh.vertices[tri.a].position);
    const auto b = to_screen(mesh.vertices[tri.b].position);
    const auto c = to_screen(mesh.vertices[tri.c].position);
    draw_line(fb, a.x, a.y, b.x, b.y, color);
    draw_line(fb, b.x, b.y, c.x, c.y, color);
    draw_line(fb, c.x, c.y, a.x, a.y, color);
  }
}

/**
 * @brief Rasterize a mesh into a framebuffer.
 *
 * Two parallel phases (see the file header): chunked vertex transform and
 * triangle cull/clip/bin (separated by a barrier), then banded scan
 * conversion. Results are identical for any worker count: bands partition
 * pixel rows disjointly, jobs run in triangle order within a band, and
 * per-pixel math does not depend on the split.
 *
 * @param mesh      Geometry to draw.
 * @param uniforms  Camera/lights/textures for shading.
 * @param options   Backface switch and worker count.
 * @param model     Model transform applied per vertex (identity = mesh space
 *                  is world space).
 * @return Per-frame counters (see obzcene::RenderStats); triangle counters
 *         are summed over the setup chunks, pixel counters over the bands.
 *
 * @note Scratch (vertex pool, per-chunk job lists and bins) lives in
 *       thread_local storage whose capacity is amortized across frames and
 *       shrunk when a much smaller mesh follows a large one.
 */
inline auto rasterize(const Mesh& mesh, const FragmentUniforms& uniforms, FrameBuffer& fb,
                      const RasterOptions& options = {}, const ModelTransform& model = {})
  -> RenderStats
{
  using raster_detail::PreparedVertex;
  using raster_detail::RasterJob;
  using raster_detail::SetupChunk;
  using raster_detail::TriCull;

  const auto fb_width  = fb.width();
  const auto fb_height = fb.height();
  const auto near_clip = uniforms.camera.near_clip();
  const auto far_clip  = uniforms.camera.far_clip();

  auto band_count = static_cast<int>(options.thread_count);
  if (band_count <= 0)
  {
    const auto hw = std::thread::hardware_concurrency();
    band_count    = static_cast<int>(hw == 0 ? 1u : hw);
  }
  band_count               = std::clamp(band_count, 1, std::max(1, fb_height / 32));
  const auto rows_per_band = (fb_height + band_count - 1) / band_count;

  thread_local raster_detail::PoolStorage pool_store;
  pool_store.shrink_if_huge(mesh.vertices.size());
  pool_store.reserve(mesh.vertices.size());
  auto pool      = pool_store.data;
  auto pool_size = mesh.vertices.size();

  thread_local std::vector<SetupChunk> chunks;
  if (chunks.size() != static_cast<usz>(band_count))
  {
    chunks.assign(static_cast<usz>(band_count), SetupChunk{});
  }
  for (auto& chunk : chunks)
  {
    // Keep inner vector capacities across frames; just reset the contents.
    chunk.stats = RenderStats{};
    chunk.jobs.clear();
    chunk.straddlers.clear();
    for (auto& bin : chunk.bins)
    {
      bin.clear();
    }
    chunk.bins.resize(static_cast<usz>(band_count));
  }

  const auto  vp          = uniforms.camera.view_projection();
  auto*       pool_data   = pool;
  const auto* mesh_ptr    = &mesh;
  auto*       chunks_data = chunks.data();

  // -- phase 1: transform (per vertex chunk), barrier, cull+clip-bin (per
  //    triangle chunk) — one spawn round for both ----------------------------
  std::barrier sync(band_count);
  threading::parallel_run(
    band_count,
    [&](int worker, int workers)
    {
      const auto vertex_begin =
        mesh_ptr->vertices.size() * static_cast<usz>(worker) / static_cast<usz>(workers);
      const auto vertex_end =
        mesh_ptr->vertices.size() * static_cast<usz>(worker + 1) / static_cast<usz>(workers);
      for (usz i = vertex_begin; i < vertex_end; ++i)
      {
        const auto& vertex = mesh_ptr->vertices[i];
        const auto  world =
          model.matrix * math::Vec4f{vertex.position.x, vertex.position.y, vertex.position.z, 1.0F};
        const auto clip = vp * world;
        auto&      out  = pool_data[i];
        out.world       = math::Vec3f{world.x, world.y, world.z};
        out.normal      = model.normal * vertex.normal;
        out.uv          = vertex.texcoord;
        raster_detail::project_vertex(out, clip, fb_width, fb_height);
      }

      sync.arrive_and_wait();

      auto& chunk = chunks_data[worker];
      chunk.bins.resize(static_cast<usz>(workers));
      const auto tri_begin =
        mesh_ptr->triangles.size() * static_cast<usz>(worker) / static_cast<usz>(workers);
      const auto tri_end =
        mesh_ptr->triangles.size() * static_cast<usz>(worker + 1) / static_cast<usz>(workers);
      for (usz ti = tri_begin; ti < tri_end; ++ti)
      {
        const auto& tri = mesh_ptr->triangles[ti];
        ++chunk.stats.triangles_processed;

        const auto& va = pool_data[tri.a];
        const auto& vb = pool_data[tri.b];
        const auto& vc = pool_data[tri.c];

        const auto in_count = static_cast<int>(va.clip_w > near_clip && va.clip_w < far_clip) +
                              static_cast<int>(vb.clip_w > near_clip && vb.clip_w < far_clip) +
                              static_cast<int>(vc.clip_w > near_clip && vc.clip_w < far_clip);
        if (in_count == 0)
        {
          ++chunk.stats.triangles_culled_nearfar;
          continue;
        }
        if (in_count != 3)
        {
          chunk.straddlers.push_back(narrow_cast<ui32>(ti));
          continue;
        }

        RasterJob  job;
        const auto verdict = raster_detail::classify_triangle(va, vb, vc, options.backface_cull,
                                                              fb_width, fb_height, job);
        switch (verdict)
        {
          case TriCull::Degenerate:
            ++chunk.stats.triangles_degenerate;
            continue;
          case TriCull::Backface:
            ++chunk.stats.triangles_culled_backface;
            continue;
          case TriCull::Viewport:
            ++chunk.stats.triangles_culled_viewport;
            continue;
          case TriCull::Render:
            break;
        }
        ++chunk.stats.triangles_rendered;
        job.a              = tri.a;
        job.b              = tri.b;
        job.c              = tri.c;
        job.material_index = tri.material_index;
        raster_detail::emit_job(chunk, job, workers, rows_per_band);
      }
    });

  // -- serial clip pass: straddlers are rare, and they append to the pool ----
  RenderStats stats;
  for (auto& chunk : chunks)
  {
    stats.triangles_processed += chunk.stats.triangles_processed;
    stats.triangles_culled_nearfar += chunk.stats.triangles_culled_nearfar;
    stats.triangles_degenerate += chunk.stats.triangles_degenerate;
    stats.triangles_culled_backface += chunk.stats.triangles_culled_backface;
    stats.triangles_culled_viewport += chunk.stats.triangles_culled_viewport;
    stats.triangles_rendered += chunk.stats.triangles_rendered;

    for (const auto ti : chunk.straddlers)
    {
      const auto& tri = mesh.triangles[ti];
      const auto& va  = pool[tri.a];
      const auto& vb  = pool[tri.b];
      const auto& vc  = pool[tri.c];

      const std::array<raster_detail::ClipVertex, 3> in_poly{
        raster_detail::ClipVertex{vp * math::Vec4f{va.world.x, va.world.y, va.world.z, 1.0F},
                                  va.world, va.normal, va.uv},
        raster_detail::ClipVertex{vp * math::Vec4f{vb.world.x, vb.world.y, vb.world.z, 1.0F},
                                  vb.world, vb.normal, vb.uv},
        raster_detail::ClipVertex{vp * math::Vec4f{vc.world.x, vc.world.y, vc.world.z, 1.0F},
                                  vc.world, vc.normal, vc.uv}};
      std::array<raster_detail::ClipVertex, 4> mid_poly;
      std::array<raster_detail::ClipVertex, 5> out_poly;
      const auto                               mid_count =
        raster_detail::clip_polygon_w(in_poly.data(), 3, near_clip, true, mid_poly.data());
      const auto out_count = mid_count == 0
                               ? usz{0}
                               : raster_detail::clip_polygon_w(mid_poly.data(), mid_count, far_clip,
                                                               false, out_poly.data());
      if (out_count < 3)
      {
        ++stats.triangles_culled_nearfar;
        continue;
      }

      const auto fan_base = narrow_cast<ui32>(pool_size);
      pool_store.reserve(pool_size + out_count);
      pool = pool_store.data;
      for (usz i = 0; i < out_count; ++i)
      {
        auto& v  = pool[pool_size];
        v.world  = out_poly[i].world;
        v.normal = out_poly[i].normal;
        v.uv     = out_poly[i].uv;
        raster_detail::project_vertex(v, out_poly[i].clip, fb_width, fb_height);
        ++pool_size;
      }
      // pool may have reallocated above; jobs store indices only, and va/vb/vc
      // are not used beyond this point in the iteration.

      // Winding/area from the first fan triangle; clipping preserves both.
      RasterJob  probe;
      const auto verdict =
        raster_detail::classify_triangle(pool[fan_base], pool[fan_base + 1], pool[fan_base + 2],
                                         options.backface_cull, fb_width, fb_height, probe);
      switch (verdict)
      {
        case TriCull::Degenerate:
          ++stats.triangles_degenerate;
          continue;
        case TriCull::Backface:
          ++stats.triangles_culled_backface;
          continue;
        case TriCull::Viewport:
          ++stats.triangles_culled_viewport;
          continue;
        case TriCull::Render:
          break;
      }

      // Union bbox over the fan decides viewport culling for the source tri.
      auto x_lo = pool[fan_base].screen.x;
      auto x_hi = x_lo;
      auto y_lo = pool[fan_base].screen.y;
      auto y_hi = y_lo;
      for (usz i = 1; i < out_count; ++i)
      {
        x_lo = std::min(x_lo, pool[fan_base + i].screen.x);
        x_hi = std::max(x_hi, pool[fan_base + i].screen.x);
        y_lo = std::min(y_lo, pool[fan_base + i].screen.y);
        y_hi = std::max(y_hi, pool[fan_base + i].screen.y);
      }
      if (std::max(static_cast<int>(x_lo), 0) > std::min(static_cast<int>(x_hi), fb_width - 1) ||
          std::max(static_cast<int>(y_lo), 0) > std::min(static_cast<int>(y_hi), fb_height - 1))
      {
        ++stats.triangles_culled_viewport;
        continue;
      }

      ++stats.triangles_rendered;
      for (usz k = 1; k + 1 < out_count; ++k)
      {
        RasterJob  job;
        const auto sub_verdict =
          raster_detail::classify_triangle(pool[fan_base], pool[fan_base + narrow_cast<ui32>(k)],
                                           pool[fan_base + narrow_cast<ui32>(k + 1)],
                                           options.backface_cull, fb_width, fb_height, job);
        if (sub_verdict != TriCull::Render)
        {
          continue; // sliver or off-screen piece of the fan
        }
        job.a              = fan_base;
        job.b              = fan_base + narrow_cast<ui32>(k);
        job.c              = fan_base + narrow_cast<ui32>(k + 1);
        job.material_index = tri.material_index;
        raster_detail::emit_job(chunk, job, band_count, rows_per_band);
      }
    }
  }

  // -- phase 2: bands scan-convert only the jobs binned into them ------------
  const auto* bins_src = chunks.data();

  struct alignas(64) BandStats
  {
    RenderStats stats{};
  };
  std::vector<BandStats> band_stats(static_cast<usz>(band_count));
  threading::parallel_run(band_count,
                          [&](int band, int workers)
                          {
                            const auto y_begin = band * rows_per_band;
                            const auto y_end   = std::min(fb_height, y_begin + rows_per_band);
                            auto       local   = RenderStats{};
                            for (auto worker = 0; worker < workers; ++worker)
                            {
                              const auto& chunk = bins_src[worker];
                              const auto  sub   = raster_detail::rasterize_band(
                                pool, chunk.jobs, chunk.bins[static_cast<usz>(band)], mesh,
                                uniforms, fb, y_begin, y_end);
                              local.pixels_shaded += sub.pixels_shaded;
                              local.pixels_written += sub.pixels_written;
                            }
                            band_stats[static_cast<usz>(band)].stats = local;
                          });

  for (const auto& band : band_stats)
  {
    stats.pixels_shaded += band.stats.pixels_shaded;
    stats.pixels_written += band.stats.pixels_written;
  }
  return stats;
}

/**
 * @brief Clear `fb` and render one frame of `mesh` into it.
 * @param fb         Target framebuffer (cleared first).
 * @param mesh       Geometry to draw.
 * @param uniforms   Camera/lights/textures for shading (lights already resolved).
 * @param wireframe  Draw outlines instead of shaded triangles.
 * @return Rasterizer statistics (empty when the mesh misses the view frustum,
 *         default-constructed beyond the clear for wireframe frames).
 * @note Shared by the interactive viewer and the snapshot path so both agree
 *       on clear colour, wireframe colour and culling options. The whole mesh
 *       is skipped cheaply when its (valid) bounds miss the camera frustum.
 */
/**
 * @brief Render one object into `fb` WITHOUT clearing (frustum-culled).
 * @param cull_bounds  World-space bounds for the whole-mesh frustum test;
 *                     defaults to `mesh.bounds` (identity model).
 * @see render_frame (single-object wrapper that clears first).
 */
inline auto render_object(FrameBuffer& fb, const Mesh& mesh, const FragmentUniforms& uniforms,
                          bool wireframe, const ModelTransform& model = {},
                          const math::AABBf* cull_bounds = nullptr) -> RenderStats
{
  const auto& bounds = cull_bounds != nullptr ? *cull_bounds : mesh.bounds;
  if (bounds.min.x <= bounds.max.x && !uniforms.camera.frustum().contains_aabb(bounds))
  {
    return {};
  }
  if (wireframe)
  {
    rasterize_wireframe(mesh, uniforms, fb, kWireframeColor, model.matrix);
    return {};
  }
  return rasterize(mesh, uniforms, fb, RasterOptions{.backface_cull = true}, model);
}

/**
 * @brief Clear `fb` and render one frame of `mesh` into it.
 * @param fb         Target framebuffer (cleared first).
 * @param mesh       Geometry to draw.
 * @param uniforms   Camera/lights/textures for shading (lights already resolved).
 * @param wireframe  Draw outlines instead of shaded triangles.
 * @param model      Model transform (identity = mesh space is world space).
 * @param cull_bounds  Optional world-space cull bounds (default: mesh.bounds).
 * @return Rasterizer statistics (empty when the mesh misses the view frustum,
 *         default-constructed beyond the clear for wireframe frames).
 * @note Shared by the interactive viewer and the snapshot path so both agree
 *       on clear colour, wireframe colour and culling options. For
 *       multi-object scenes, clear once with clear_frame() and call
 *       render_object() per object instead.
 */
inline auto render_frame(FrameBuffer& fb, const Mesh& mesh, const FragmentUniforms& uniforms,
                         bool wireframe, const ModelTransform& model = {},
                         const math::AABBf* cull_bounds = nullptr) -> RenderStats
{
  fb.clear(kClearColor);
  fb.clear_depth();
  return render_object(fb, mesh, uniforms, wireframe, model, cull_bounds);
}

/** @brief Clear colour/depth only; pair with render_object() for multi-object frames. */
inline auto clear_frame(FrameBuffer& fb) -> void
{
  fb.clear(kClearColor);
  fb.clear_depth();
}

} // namespace obzcene
