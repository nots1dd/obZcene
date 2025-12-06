#include "SDL/Render/renderutils.h"
#include "Math/clamp.h"
#include <math.h>

static const float INV_255 = 0.00392156862745098f; // 1 / 255

void __OBZ_barycentric_persp(Vec3 v0, Vec3 v1, Vec3 v2, int x, int y, float* w0, float* w1,
                             float* w2)
{
  // ---------------------- BARYCENTRIC COORDS (2D) ----------------------
  /*
      Compute barycentric coordinates of pixel (x,y) in triangle (v0, v1, v2).

      Using area ratios:
          w0 = area(P, v1, v2) / area(v0, v1, v2)
          w1 = area(P, v2, v0) / area(v0, v1, v2)
          w2 = area(P, v0, v1) / area(v0, v1, v2)

      The denominator is a 2D cross product:
          denom = (v1 - v0) × (v2 - v0)

      If denom ≈ 0 ⇒ triangle is degenerate or extremely thin.
  */
  // ----------------------------------------------------------------------
  const float denom = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
  if (fabsf(denom) < 1e-6f)
  {
    *w0 = *w1 = *w2 = -1.0f;
    return;
  }

  const float invDen = 1.0f / denom;

  *w0 = ((v1.x - v0.x) * (y - v0.y) - (v1.y - v0.y) * (x - v0.x)) * invDen;
  *w1 = ((v2.x - v1.x) * (y - v1.y) - (v2.y - v1.y) * (x - v1.x)) * invDen;
  *w2 = 1.0f - *w0 - *w1;
}

Vec2 __OBZ_interp_uv_persp(Vec2 uv0, Vec2 uv1, Vec2 uv2, float one_by_z0, float one_by_z1,
                           float one_by_z2, float w0, float w1, float w2)
{
  // ---------------------- PERSPECTIVE-CORRECT INTERPOLATION ----------------------
  /*
      UV interpolation must respect perspective.

      Instead of:
          uv = w0 * uv0 + w1 * uv1 + w2 * uv2      (WRONG for 3D)

      We use:
          (u/z) interpolates linearly
          (v/z) interpolates linearly
          (1/z) interpolates linearly

      Then reconstruct:
          u = (Σ wi * (ui / zi)) / (Σ wi * (1/zi))
          v = (Σ wi * (vi / zi)) / (Σ wi * (1/zi))

      This prevents texture distortion on steep surfaces.
  */
  // -------------------------------------------------------------------------------
  const float u0  = uv0.x * one_by_z0;
  const float v0_ = uv0.y * one_by_z0;
  const float u1  = uv1.x * one_by_z1;
  const float v1_ = uv1.y * one_by_z1;
  const float u2  = uv2.x * one_by_z2;
  const float v2_ = uv2.y * one_by_z2;

  float u = w0 * u0 + w1 * u1 + w2 * u2;
  float v = w0 * v0_ + w1 * v1_ + w2 * v2_;

  // Interpolated inverse-depth
  float iz = 1.0f / (w0 * one_by_z0 + w1 * one_by_z1 + w2 * one_by_z2);

  Vec2 uv = {u * iz, v * iz};

  return uv;
}

bool __OBZ_triangle_offscreen(Vec3 p0, Vec3 p1, Vec3 p2, int W, int H)
{
  // ---------------------- TRIVIAL FRUSTUM REJECTION ----------------------
  /*
      Quickly discard triangles completely outside screen bounds.

      Each test checks if ALL vertices lie on the same outside region:
          All left, all right, all above, all below ⇒ offscreen.

      Also reject if any z <= 0 ⇒ behind camera (invalid in view space).
  */
  // ------------------------------------------------------------------------
  if (p0.x < 0 && p1.x < 0 && p2.x < 0)
    return true;
  if (p0.x >= W && p1.x >= W && p2.x >= W)
    return true;
  if (p0.y < 0 && p1.y < 0 && p2.y < 0)
    return true;
  if (p0.y >= H && p1.y >= H && p2.y >= H)
    return true;

  if (p0.z <= 0 || p1.z <= 0 || p2.z <= 0)
    return true;

  return false;
}

OBZ_pixel __OBZ_sample_texture(const OBZ_Texture* tex, Vec2 uv, OBZ_Color diffuse)
{
  // ---------------------- TEXTURE SAMPLING (UV MAPPING) ----------------------
  /*
      UV coordinates ∈ [0,1].

      Mapping to texture pixel:
          x = u * (width  - 1)
          y = (1 - v) * (height - 1)     // flip vertically (OpenGL-style)

      Color modulation:
          tex_color * diffuse_color / 255

      Using precomputed INV_255 = 1 / 255 saves divisions (faster).
  */
  // --------------------------------------------------------------------------
  if (!tex || !tex->pixels)
    return __OBZ_pack_color(diffuse);

  float u = obz_clampf01(uv.x);
  float v = obz_clampf01(uv.y);

  int tx = (int)(u * (tex->width - 1));
  int ty = (int)((1.0f - v) * (tex->height - 1));

  OBZ_channel* p = tex->pixels + 4 * (ty * tex->width + tx);

  auto r = (OBZ_channel)((p[0] * diffuse.r) * INV_255);
  auto g = (OBZ_channel)((p[1] * diffuse.g) * INV_255);
  auto b = (OBZ_channel)((p[2] * diffuse.b) * INV_255);
  auto a = p[3];

  return __OBZ_pack_color_channels(r, g, b, a);
}

void __OBZ_render_clear_depth_buffer(OBZ_DynArray* zbuf, const int n, const float* zbuf_clear_ptr)
{
  // ---------------------- DEPTH BUFFER CLEAR ----------------------
  /*
      Depth buffers hold depth values per pixel.

      Clearing means filling the entire buffer with a chosen depth value:

          z[i] = clear_value

      This restores the buffer before rendering a new frame.
  */
  // ----------------------------------------------------------------
  if (obz_arr_exists(zbuf))
  {
    for (int i = 0; i < n; i++)
      obz_arr_set(zbuf, i, zbuf_clear_ptr);
  }
}
