#include "SDL/Render/renderutils.h"
#include "Math/abs.h"

static const float INV_255 = 0.00392156862745098f; // 1 / 255

Vec3f __OBZ_barycentric_persp(Vec3f a, Vec3f b, Vec3f c, int px, int py)
{
  Vec3f result = {0};

  float acx = c.x - a.x, acy = c.y - a.y;
  float abx = b.x - a.x, aby = b.y - a.y;
  float apx = px - a.x, apy = py - a.y;

  float area = (acx * aby - acy * abx);
  if (obz_abs(area) < 1e-12f)
  {
    result.x = result.y = result.z = -1;
    return result;
  }

  // w0 = alpha
  float pcx = c.x - px, pcy = c.y - py;
  float pbx = b.x - px, pby = b.y - py;
  float alpha = (pcx * pby - pcy * pbx) / area;

  // w1 = beta
  float beta = (acx * apy - acy * apx) / area;

  // w2 = gamma
  float gamma = 1.0f - alpha - beta;

  result.x = alpha;
  result.y = beta;
  result.z = gamma;

  return result;
}

Vec2f __OBZ_interp_uv_persp(Vec2f uv0, Vec2f uv1, Vec2f uv2, float iz0, float iz1, float iz2,
                            float w0, float w1, float w2)
{
  const float u  = uv0.x * iz0 * w0 + uv1.x * iz1 * w1 + uv2.x * iz2 * w2;
  const float v  = uv0.y * iz0 * w0 + uv1.y * iz1 * w1 + uv2.y * iz2 * w2;
  const float iz = iz0 * w0 + iz1 * w1 + iz2 * w2;

  const float f = 1.0f / iz;

  return (Vec2f){u * f, v * f};
}

bool __OBZ_triangle_offscreen(Vec3f p0, Vec3f p1, Vec3f p2, int W, int H)
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

OBZ_pixel __OBZ_sample_texture(const OBZ_Texture* tex, Vec2f uv, OBZ_Color diffuse)
{
  if (!tex || !tex->pixels)
    return __OBZ_pack_color(diffuse);

  float u = uv.x - obz_floorf(uv.x); // fract
  float v = uv.y - obz_floorf(uv.y);
  v       = 1.0f - v; // flip V

  float fx = u * (tex->width - 1);
  float fy = v * (tex->height - 1);

  int x0 = obz_clampi((int)fx, 0, tex->width - 1);
  int x1 = obz_clampi(x0 + 1, 0, tex->width - 1);
  int y0 = obz_clampi((int)fy, 0, tex->height - 1);
  int y1 = obz_clampi(y0 + 1, 0, tex->height - 1);

  float sx = fx - x0;
  float sy = fy - y0;

  OBZ_channel* p00 = tex->pixels + 4 * (y0 * tex->width + x0);
  OBZ_channel* p10 = tex->pixels + 4 * (y0 * tex->width + x1);
  OBZ_channel* p01 = tex->pixels + 4 * (y1 * tex->width + x0);
  OBZ_channel* p11 = tex->pixels + 4 * (y1 * tex->width + x1);

  float r0 = p00[0] * (1 - sx) + p10[0] * sx;
  float r1 = p01[0] * (1 - sx) + p11[0] * sx;
  float g0 = p00[1] * (1 - sx) + p10[1] * sx;
  float g1 = p01[1] * (1 - sx) + p11[1] * sx;
  float b0 = p00[2] * (1 - sx) + p10[2] * sx;
  float b1 = p01[2] * (1 - sx) + p11[2] * sx;
  float a0 = p00[3] * (1 - sx) + p10[3] * sx;
  float a1 = p01[3] * (1 - sx) + p11[3] * sx;

  float r = r0 * (1 - sy) + r1 * sy;
  float g = g0 * (1 - sy) + g1 * sy;
  float b = b0 * (1 - sy) + b1 * sy;
  float a = a0 * (1 - sy) + a1 * sy;

  return __OBZ_pack_color_channels((OBZ_channel)(r * diffuse.r * INV_255),
                                   (OBZ_channel)(g * diffuse.g * INV_255),
                                   (OBZ_channel)(b * diffuse.b * INV_255), (OBZ_channel)(a));
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
