#include "SDL/Render/renderutils.h"
#include "Math/clamp.h"
#include <math.h>

static const float INV_255 = 0.00392156862745098f; // 1 / 255

void __OBZ_barycentric_persp(Vec3 v0, Vec3 v1, Vec3 v2, int x, int y, float* w0, float* w1,
                             float* w2)
{
  // ---------------------- BARYCENTRIC COORDS (2D) ------------------------------------------------
  /*
      Compute barycentric coordinates of the pixel center (x+0.5, y+0.5) in triangle (v0, v1, v2).

      Using canonical edge functions:
          edge(a,b,p) = (b.x - a.x)*(p.y - a.y) - (b.y - a.y)*(p.x - a.x)

      Denominator (area*2) computed as:
          denom = edge(v0, v1, v2)

      Use double precision for numerical stability, and scale-aware epsilon:
          area_eps = 1e-6 * (1 + max(|v0.x|, |v0.y|, |v1.x|, |v1.y|, |v2.x|, |v2.y|))

      Reject degenerate triangles:
          if |denom| < area_eps ⇒ set weights to -1

      Compute edge functions for pixel center:
          e0 = edge(v1, v2, P)  // weight for v0
          e1 = edge(v2, v0, P)  // weight for v1
          e2 = edge(v0, v1, P)  // weight for v2

      Normalize orientation to ensure consistent winding:
          sign = +1 if denom > 0 else -1
          se_i = e_i * sign

      Apply top-left rule for shared-edge correctness:
          accept edge if se_i > 0 or (se_i == 0 and is_top_left_edge(a,b))

      If any edge is not accepted, pixel is outside triangle ⇒ weights = -1

      Otherwise, compute normalized barycentric coordinates:
          w0 = e0 / denom
          w1 = e1 / denom
          w2 = 1 - w0 - w1

      This produces robust coverage with correct edge rules and numerical stability.
  */
  // --------------------------------------------------------------------------------------------

  const double px = x + 0.5;
  const double py = y + 0.5;

  // Compute triangle "area" (denominator) in double for stability
  const double denom = __OBZ_edge_func_d(&v0, &v1, v2.x, v2.y);

  // Scale-aware epsilon: max coord among triangle vertices
  double       max_coord = fmax(fmax(fabs(v0.x), fabs(v0.y)),
                                fmax(fmax(fabs(v1.x), fabs(v1.y)), fmax(fabs(v2.x), fabs(v2.y))));
  const double area_eps  = 1e-6 * (1.0 + max_coord);

  if (fabs(denom) < area_eps)
  {
    *w0 = *w1 = *w2 = -1.0f;
    return;
  }

  // Edge functions (for barycentric weights)
  const double e0 = __OBZ_edge_func_d(&v1, &v2, px, py); // for w0
  const double e1 = __OBZ_edge_func_d(&v2, &v0, px, py); // for w1

  // Normalize orientation to ensure consistent winding
  const double sign = denom > 0.0 ? 1.0 : -1.0;
  const double se0  = e0 * sign;
  const double se1  = e1 * sign;
  const double se2  = (denom - e0 - e1) * sign;

  // Top-left edge inclusion (inline to avoid branching function call)
  int accept0 = (se0 > 0.0) || (se0 == 0.0 && ((v2.y < v1.y) || (v2.y == v1.y && v2.x > v1.x)));
  int accept1 = (se1 > 0.0) || (se1 == 0.0 && ((v0.y < v2.y) || (v0.y == v2.y && v0.x > v2.x)));
  int accept2 = (se2 > 0.0) || (se2 == 0.0 && ((v1.y < v0.y) || (v1.y == v0.y && v1.x > v0.x)));

  if (!(accept0 && accept1 && accept2))
  {
    *w0 = *w1 = *w2 = -1.0f;
    return;
  }

  const double invDen = 1.0 / denom;
  *w0                 = (float)(e0 * invDen);
  *w1                 = (float)(e1 * invDen);
  *w2                 = 1.0f - *w0 - *w1;
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
  const float u = w0 * (uv0.x * one_by_z0) + w1 * (uv1.x * one_by_z1) + w2 * (uv2.x * one_by_z2);
  const float v = w0 * (uv0.y * one_by_z0) + w1 * (uv1.y * one_by_z1) + w2 * (uv2.y * one_by_z2);

  // Denominator for perspective-correct interpolation
  float       denom = w0 * one_by_z0 + w1 * one_by_z1 + w2 * one_by_z2;
  float       mag   = fmax(fmax(fabsf(one_by_z0), fabsf(one_by_z1)), fabsf(one_by_z2));
  const float eps   = 1e-8f * fmax(1.0f, mag);

  if (denom <= eps)
  {
    // fallback to affine interpolation
    return (Vec2){w0 * uv0.x + w1 * uv1.x + w2 * uv2.x, w0 * uv0.y + w1 * uv1.y + w2 * uv2.y};
  }

  float iz = 1.0f / denom;
  return (Vec2){u * iz, v * iz};
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
  return ((p0.x < 0 && p1.x < 0 && p2.x < 0) || (p0.x >= W && p1.x >= W && p2.x >= W) ||
          (p0.y < 0 && p1.y < 0 && p2.y < 0) || (p0.y >= H && p1.y >= H && p2.y >= H) ||
          (p0.z <= 0 && p1.z <= 0 && p2.z <= 0));
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

  // Clamp UV to [0,1]
  float u = obz_clampf01(uv.x);
  float v = obz_clampf01(uv.y);

  int tx = (int)(u * (tex->width - 1));
  int ty = (int)((1.0f - v) * (tex->height - 1));

  // Clamp indices
  if (tx < 0)
    tx = 0;
  else if (tx >= (int)tex->width)
    tx = tex->width - 1;
  if (ty < 0)
    ty = 0;
  else if (ty >= (int)tex->height)
    ty = tex->height - 1;

  OBZ_channel* p = tex->pixels + 4 * (ty * tex->width + tx);

  // Multiply channels and diffuse using precomputed INV_255
  return __OBZ_pack_color_channels((OBZ_channel)(p[0] * diffuse.r * INV_255),
                                   (OBZ_channel)(p[1] * diffuse.g * INV_255),
                                   (OBZ_channel)(p[2] * diffuse.b * INV_255), p[3]);
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
