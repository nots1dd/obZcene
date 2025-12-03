#include "SDL/Render/render.h"
#include "Math/clamp.h"
#include "Math/minimax.h"
#include "obz_log.h"
#include "obz_mem.h"
#include "obz_types.h"
#include <alloca.h>
#include <math.h>

OBZ_RendererContext* obz_render_context_init(int width, int height)
{
  OBZ_RendererContext* renctx = obz_malloc(sizeof(*renctx));
  if (!renctx)
  {
    OBZ_LOG_ERROR(NULL, "Failed to allocate RendererContext.");
    free(renctx);
    return NULL;
  }

  renctx->width  = width;
  renctx->height = height;

  // count is just size_t alias; easy to understand when calling obz_arr apis
  const obz_count_t pixel_count = (size_t)(width * height);

  renctx->framebuffer = obz_malloc(pixel_count * 4); // RGBA colors (!!ABGR FORMAT!!)
  if (!renctx->framebuffer)
  {
    OBZ_LOG_ERROR(NULL, "Failed to allocate framebuffer.");
    obz_free(renctx);
    return NULL;
  }

  // Float Z-buffer
  renctx->zbuffer = obz_arr_create(sizeof(float));
  obz_arr_reserve(&renctx->zbuffer, pixel_count);

  OBZ_ASSERT(obz_arr_exists(&renctx->zbuffer) == false,
             "Something went wrong allocating z-buffer!");
  OBZ_ASSERT(renctx != NULL, "Renderer context is NULL after allocation!");

  OBZ_LOG_INFO(NULL, "obZcene renderer context created successfully (w: %d, h: %d).", width,
               height);

  return renctx;
}

void __OBZ_render_clear_zbuf(OBZ_DynArray* zbuf, const int n, const float* zbuf_clear_ptr)
{
  if (obz_arr_exists(zbuf))
  {
    for (int i = 0; i < n; i++)
      obz_arr_set(zbuf, i, zbuf_clear_ptr);
  }
}

void obz_render_clear(OBZ_RendererContext* ctx, OBZ_pixel clear_color)
{
  const int n = ctx->width * ctx->height;

  // Clear framebuffer
  for (int i = 0; i < n; i++)
    ctx->framebuffer[i] = clear_color;

  // Clear z-buffer if it exists
  const float z_clear = OBZ_Z_BUF_CLEAR;
  __OBZ_render_clear_zbuf(&ctx->zbuffer, n, &z_clear);
}

// Put pixel with depth test
void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color)
{
  if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
    return;

  int idx = y * ctx->width + x;

  if (obz_arr_exists(&ctx->zbuffer))
  {
    auto p_z = (float*)obz_arr_get(&ctx->zbuffer, idx);
    if (!p_z)
      return;

    if (z > *p_z)
      return;

    obz_arr_set(&ctx->zbuffer, idx, &z);
  }

  ctx->framebuffer[idx] = color;
}

inline static void __OBZ_barycentric_persp(Vec3 v0, Vec3 v1, Vec3 v2, int x, int y, float* w0,
                                           float* w1, float* w2)
{
  const float denom = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
  if (fabsf(denom) < 1e-6f)
  {
    *w0 = *w1 = *w2 = -1.0f;
    return;
  }
  const float invDen = 1.0f / denom;
  *w0                = ((v1.x - v0.x) * (y - v0.y) - (v1.y - v0.y) * (x - v0.x)) * invDen;
  *w1                = ((v2.x - v1.x) * (y - v1.y) - (v2.y - v1.y) * (x - v1.x)) * invDen;
  *w2                = 1.0f - *w0 - *w1;
}

// Perspective UV interpolation
static Vec2 __OBZ_interp_uv_persp(Vec3 v0, Vec3 v1, Vec3 v2, Vec2 uv0, Vec2 uv1, Vec2 uv2, float w0,
                                  float w1, float w2)
{
  // Divide UV by vertex depth
  const float u0  = uv0.x / v0.z;
  const float v0_ = uv0.y / v0.z;
  const float u1  = uv1.x / v1.z;
  const float v1_ = uv1.y / v1.z;
  const float u2  = uv2.x / v2.z;
  const float v2_ = uv2.y / v2.z;

  // Interpolate
  float u = w0 * u0 + w1 * u1 + w2 * u2;
  float v = w0 * v0_ + w1 * v1_ + w2 * v2_;

  // Interpolate 1/z
  float iz = w0 / v0.z + w1 / v1.z + w2 / v2.z;

  Vec2 uv = {u / iz, v / iz};

  return uv;
}

inline static OBZ_pixel __OBZ_sample_texture(const OBZ_Texture* tex, Vec2 uv, OBZ_Color diffuse)
{
  if (!tex || !tex->pixels)
    return __OBZ_pack_color_channels(diffuse.r, diffuse.g, diffuse.b, diffuse.a);

  float        u  = obz_clampf01(uv.x);
  float        v  = obz_clampf01(uv.y);
  auto         tx = (int)(u * (tex->width - 1));
  auto         ty = (int)((1.0f - v) * (tex->height - 1));
  OBZ_channel* p  = tex->pixels + 4 * (ty * tex->width + tx); // ABGR

  auto r = (OBZ_channel)((p[0] * diffuse.r) / 255);
  auto g = (OBZ_channel)((p[1] * diffuse.g) / 255);
  auto b = (OBZ_channel)((p[2] * diffuse.b) / 255);
  auto a = p[3];

  return __OBZ_pack_color_channels(r, g, b, a);
}

static void __OBZ_render_triangle(OBZ_RendererContext* ctx, Vec3 v0, Vec3 v1, Vec3 v2, Vec2 uv0,
                                  Vec2 uv1, Vec2 uv2, OBZ_Texture* tex, OBZ_Color diffuse,
                                  int use_color_only)
{
  if (!ctx)
    return;

  float dx1 = v1.x - v0.x;
  float dy1 = v1.y - v0.y;
  float dx2 = v2.x - v0.x;
  float dy2 = v2.y - v0.y;
  if ((dx1 * dy2 - dx2 * dy1) <= 0.0f)
    return;

  int minx = obz_clamp_floor_to_int(obz_minf(obz_minf(v0.x, v1.x), v2.x), 0, ctx->width - 1);
  int maxx = obz_clamp_ceil_to_int(obz_maxf(obz_maxf(v0.x, v1.x), v2.x), 0, ctx->width - 1);
  int miny = obz_clamp_floor_to_int(obz_minf(obz_minf(v0.y, v1.y), v2.y), 0, ctx->height - 1);
  int maxy = obz_clamp_ceil_to_int(obz_maxf(obz_maxf(v0.y, v1.y), v2.y), 0, ctx->height - 1);
  if (minx > maxx || miny > maxy)
    return;

  auto zb = obz_arr_exists(&ctx->zbuffer) ? obz_arr_get_data(&ctx->zbuffer, float) : NULL;

  for (int y = miny; y <= maxy; ++y)
  {
    for (int x = minx; x <= maxx; ++x)
    {
      float w0, w1, w2;
      __OBZ_barycentric_persp(v0, v1, v2, x, y, &w0, &w1, &w2);
      if (w0 < 0 || w1 < 0 || w2 < 0)
        continue;

      float z   = w0 * v0.z + w1 * v1.z + w2 * v2.z;
      int   idx = y * ctx->width + x;

      if (zb && z > zb[idx])
        continue;
      if (zb)
        zb[idx] = z;

      OBZ_pixel out;
      if (use_color_only || !tex || !tex->pixels)
      {
        out = __OBZ_pack_color_channels(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
      }
      else
      {
        Vec2 uv = __OBZ_interp_uv_persp(v0, v1, v2, uv0, uv1, uv2, w0, w1, w2);
        out     = __OBZ_sample_texture(tex, uv, diffuse);
      }

      ctx->framebuffer[idx] = out;
    }
  }
}

void obz_render_mesh_camera(OBZ_RendererContext* ctx, Vec3 pos, OBZ_Mesh3D* mesh, float pitch,
                            float yaw, float roll, OBZ_Camera cam)
{
  if (!ctx || !mesh)
    return;

  const int nverts = mesh->noOfVerts;
  const int nfaces = mesh->noOfFaces;
  if (nverts == 0 || nfaces == 0)
    return;

  Vec3* verts_world  = obz_malloc(nverts * sizeof(Vec3));
  Vec3* verts_screen = obz_malloc(nverts * sizeof(Vec3));

  /* Precompute rotation */
  float cp = cosf(pitch), sp = sinf(pitch);
  float cy = cosf(yaw), sy = sinf(yaw);
  float cr = cosf(roll), sr = sinf(roll);

  /* Transform vertices */
  for (int i = 0; i < nverts; i++)
  {
    Vec3 v = mesh->verts[i];

    float x1 = v.x * cy - v.z * sy;
    float z1 = v.x * sy + v.z * cy;
    float y2 = v.y * cp - z1 * sp;
    float z2 = v.y * sp + z1 * cp;
    float x3 = x1 * cr - y2 * sr;
    float y3 = x1 * sr + y2 * cr;

    Vec3 world     = {x3 + pos.x, y3 + pos.y, z2 + pos.z};
    verts_world[i] = world;

    int sx, syi;

    obz_project_camera(world, cam, &sx, &syi, ctx->width, ctx->height);
    verts_screen[i] = (Vec3){(float)sx, (float)syi, world.z};
  }

  /* Fallback 64% grey texture and white diffuse */
  static OBZ_channel fallback_px[4]   = {160, 160, 160, 255};
  static OBZ_Texture fallback_tex     = {.pixels = fallback_px, .width = 1, .height = 1};
  static OBZ_Color   fallback_diffuse = {255, 255, 255, 255};

  int* vidx = mesh->indices;
  int* uidx = mesh->uv_indices;

  for (int t = 0; t < nfaces; t++)
  {
    int i0 = vidx[t * 3 + 0];
    int i1 = vidx[t * 3 + 1];
    int i2 = vidx[t * 3 + 2];

    if ((unsigned)i0 >= (unsigned)nverts || (unsigned)i1 >= (unsigned)nverts ||
        (unsigned)i2 >= (unsigned)nverts)
      continue;

    Vec3 p0 = verts_screen[i0];
    Vec3 p1 = verts_screen[i1];
    Vec3 p2 = verts_screen[i2];

    /* Material / texture */
    OBZ_Texture* tex     = &fallback_tex;
    OBZ_Color    diffuse = fallback_diffuse;

    int mid = mesh->face_mtl_id ? mesh->face_mtl_id[t] : -1;
    if (mid >= 0 && mid < mesh->material_count)
    {
      OBZ_ObjMtl* M = &mesh->materials[mid];
      if (M->map_Kd)
        tex = M->map_Kd; // texture
      diffuse.r = __OBZ_clampf_to_channel(M->Kd.x);
      diffuse.g = __OBZ_clampf_to_channel(M->Kd.y);
      diffuse.b = __OBZ_clampf_to_channel(M->Kd.z);
      diffuse.a = 255;
    }

    /* UVs */
    Vec2 uv0 = {0, 0}, uv1 = {0, 0}, uv2 = {0, 0};
    if (mesh->uvs && mesh->uv_indices)
    {
      int ti0 = uidx[t * 3 + 0];
      int ti1 = uidx[t * 3 + 1];
      int ti2 = uidx[t * 3 + 2];

      int nuv = mesh->noOfUVs;
      if ((unsigned)ti0 < (unsigned)nuv)
        uv0 = mesh->uvs[ti0];
      if ((unsigned)ti1 < (unsigned)nuv)
        uv1 = mesh->uvs[ti1];
      if ((unsigned)ti2 < (unsigned)nuv)
        uv2 = mesh->uvs[ti2];
    }

    __OBZ_render_triangle(ctx, p0, p1, p2, uv0, uv1, uv2, tex, diffuse, 0);
  }
}
