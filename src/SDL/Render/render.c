#include "SDL/Render/render.h"
#include "Math/clamp.h"
#include "Math/minimax.h"
#include "Utils/arena_allocator.h"
#include "obz_log.h"
#include "obz_mem.h"
#include "obz_types.h"
#include <math.h>

static const float INV_255 = 0.00392156862745098f; // 1 / 255

/* Fallback 64% grey texture and white diffuse */
static OBZ_channel fallback_px[4] = {160, 160, 160, 255};
static OBZ_Texture fallback_tex   = {.pixels = fallback_px, .width = 1, .height = 1};

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

  // obz_count_t is alias to size_t; easier to understand when calling `obz_arr_` API
  const obz_count_t pixel_count = (width * height);

  renctx->framebuffer =
    obz_malloc(pixel_count * sizeof(OBZ_pixel)); // RGBA colors (!!ABGR FORMAT!!)
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

  // clear framebuffer
  for (int i = 0; i < n; i++)
    ctx->framebuffer[i] = clear_color;

  // clear zbuf if it exists
  const float z_clear = OBZ_Z_BUF_CLEAR;
  __OBZ_render_clear_zbuf(&ctx->zbuffer, n, &z_clear);
}

void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color)
{
  if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
    return;

  int idx = y * ctx->width + x;

  float* zb = obz_arr_exists(&ctx->zbuffer) ? obz_arr_get_data(&ctx->zbuffer, float) : NULL;

  if (zb)
  {
    if (z > zb[idx])
      return;
    zb[idx] = z;
  }

  ctx->framebuffer[idx] = color;
}

static void __OBZ_barycentric_persp(Vec3 v0, Vec3 v1, Vec3 v2, int x, int y, float* w0, float* w1,
                                    float* w2)
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

static Vec2 __OBZ_interp_uv_persp(Vec2 uv0, Vec2 uv1, Vec2 uv2, float one_by_z0, float one_by_z1,
                                  float one_by_z2, float w0, float w1, float w2)
{
  // Divide UV by vertex depth using pre-computed reciprocals
  const float u0  = uv0.x * one_by_z0;
  const float v0_ = uv0.y * one_by_z0;
  const float u1  = uv1.x * one_by_z1;
  const float v1_ = uv1.y * one_by_z1;
  const float u2  = uv2.x * one_by_z2;
  const float v2_ = uv2.y * one_by_z2;

  // Interpolate
  float u = w0 * u0 + w1 * u1 + w2 * u2;
  float v = w0 * v0_ + w1 * v1_ + w2 * v2_;

  // Interpolate 1/z
  float iz = 1.0f / (w0 * one_by_z0 + w1 * one_by_z1 + w2 * one_by_z2);

  Vec2 uv = {u * iz, v * iz};

  return uv;
}

static OBZ_pixel __OBZ_sample_texture(const OBZ_Texture* tex, Vec2 uv, OBZ_Color diffuse)
{
  if (!tex || !tex->pixels)
    return __OBZ_pack_color(diffuse);

  float        u  = obz_clampf01(uv.x);
  float        v  = obz_clampf01(uv.y);
  auto         tx = (int)(u * (tex->width - 1));
  auto         ty = (int)((1.0f - v) * (tex->height - 1));
  OBZ_channel* p  = tex->pixels + 4 * (ty * tex->width + tx);

  // Use multiply by reciprocal instead of division
  auto r = (OBZ_channel)((p[0] * diffuse.r) * INV_255);
  auto g = (OBZ_channel)((p[1] * diffuse.g) * INV_255);
  auto b = (OBZ_channel)((p[2] * diffuse.b) * INV_255);
  auto a = p[3];

  return __OBZ_pack_color_channels(r, g, b, a);
}

static void __OBZ_render_triangle(OBZ_RendererContext* ctx, Vec3 v0, Vec3 v1, Vec3 v2, Vec2 uv0,
                                  Vec2 uv1, Vec2 uv2, float one_by_z0, float one_by_z1,
                                  float one_by_z2, OBZ_Texture* tex, OBZ_Color diffuse,
                                  int use_color_only)
{
  if (!ctx)
    return;

  // Compute bounding box of the triangle
  int minx = obz_clamp_floor_to_int(obz_minf(obz_minf(v0.x, v1.x), v2.x), 0, ctx->width - 1);
  int maxx = obz_clamp_ceil_to_int(obz_maxf(obz_maxf(v0.x, v1.x), v2.x), 0, ctx->width - 1);
  int miny = obz_clamp_floor_to_int(obz_minf(obz_minf(v0.y, v1.y), v2.y), 0, ctx->height - 1);
  int maxy = obz_clamp_ceil_to_int(obz_maxf(obz_maxf(v0.y, v1.y), v2.y), 0, ctx->height - 1);

  if (minx > maxx || miny > maxy)
    return;

  // zbuffer pointer once
  auto zb = obz_arr_exists(&ctx->zbuffer) ? obz_arr_get_data(&ctx->zbuffer, float) : NULL;

  // Pre-compute color for solid-color triangles
  OBZ_pixel solid_color_pixels = 0;
  if (use_color_only || !tex || !tex->pixels)
  {
    solid_color_pixels = __OBZ_pack_color(diffuse);
  }

  for (int y = miny; y <= maxy; ++y)
  {
    for (int x = minx; x <= maxx; ++x)
    {
      float w0, w1, w2;
      __OBZ_barycentric_persp(v0, v1, v2, x, y, &w0, &w1, &w2);

      // outside triangle
      if (w0 < 0 || w1 < 0 || w2 < 0)
        continue;

      float z   = w0 * v0.z + w1 * v1.z + w2 * v2.z;
      int   idx = y * ctx->width + x;

      // Depth test BEFORE texture sampling
      if (zb)
      {
        if (z > zb[idx])
          continue;
        zb[idx] = z;
      }

      OBZ_pixel out_buffer;

      // texture sampling only after depth test passes
      if (use_color_only || !tex || !tex->pixels)
      {
        out_buffer = solid_color_pixels;
      }
      else
      {
        Vec2 uv = __OBZ_interp_uv_persp(uv0, uv1, uv2, one_by_z0, one_by_z1, one_by_z2, w0, w1, w2);
        out_buffer = __OBZ_sample_texture(tex, uv, diffuse);
      }

      ctx->framebuffer[idx] = out_buffer;
    }
  }
}

bool __OBZ_triangle_offscreen(Vec3 p0, Vec3 p1, Vec3 p2, int W, int H)
{
  // Left
  if (p0.x < 0 && p1.x < 0 && p2.x < 0)
    return true;
  // Right
  if (p0.x >= W && p1.x >= W && p2.x >= W)
    return true;
  // Top
  if (p0.y < 0 && p1.y < 0 && p2.y < 0)
    return true;
  // Bottom
  if (p0.y >= H && p1.y >= H && p2.y >= H)
    return true;

  // Behind camera
  if (p0.z <= 0 || p1.z <= 0 || p2.z <= 0)
    return true;

  return false;
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

  Vec3* verts_world =
    obz_arena_alloc(ctx->arena_alloc, nverts * sizeof(Vec3), OBZ_ARENA_DEFAULT_ALIGNMENT);
  Vec3* verts_screen =
    obz_arena_alloc(ctx->arena_alloc, nverts * sizeof(Vec3), OBZ_ARENA_DEFAULT_ALIGNMENT);
  float* one_by_z =
    obz_arena_alloc(ctx->arena_alloc, nverts * sizeof(float), OBZ_ARENA_DEFAULT_ALIGNMENT);

  float cp = cosf(pitch), sp = sinf(pitch);
  float cy = cosf(yaw), sy = sinf(yaw);
  float cr = cosf(roll), sr = sinf(roll);

  /* Camera forward (ensure normalized) */
  Vec3 cam_fwd = obz_vec3_norm(cam.direction);

  /* Transform vertices:
     - compute world-space position (world)
     - compute view-space depth vz = dot(world - cam.position, cam_fwd)
     - store screen X/Y from obz_project_camera, and store vz into verts_screen[i].z
     - precompute one_by_z = 1.0f / vz (or 0 if vz <= 0)
  */
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

    // view vector from camera to world vertex
    Vec3  view_vec = obz_vec3_sub(world, cam.position);
    float vz       = obz_vec3_dot(view_vec, cam_fwd); // depth along camera forward

    // Project using existing projection routine (expects world + cam)
    int sx = 0, sy = 0;
    if (obz_project_camera(world, cam, &sx, &sy, ctx->width, ctx->height) ==
        OBZ_CAMERA_OUT_OF_BOUNDS)
    {
      // Failed projection
      continue;
    }

    // store screen coords and view-space depth in z
    verts_screen[i] = (Vec3){(float)sx, (float)sy, vz};

    one_by_z[i] = (vz > 0.0f) ? (1.0f / vz) : 0.0f;
  }

  int* vidx = mesh->indices;
  int* uidx = mesh->uv_indices;

  int          current_material_id = -2; // -2 means no material set yet
  OBZ_Texture* current_tex         = &fallback_tex;
  OBZ_Color    current_diffuse     = COLOR_WHITE;

  for (int t = 0; t < nfaces; t++)
  {
    int i0 = vidx[t * 3 + 0];
    int i1 = vidx[t * 3 + 1];
    int i2 = vidx[t * 3 + 2];

    if ((unsigned)i0 >= (unsigned)nverts || (unsigned)i1 >= (unsigned)nverts ||
        (unsigned)i2 >= (unsigned)nverts)
      continue;

    /* Screen-space vertices (z contains view-space depth vz) */
    Vec3 p0 = verts_screen[i0];
    Vec3 p1 = verts_screen[i1];
    Vec3 p2 = verts_screen[i2];

    /* Early rejection for off-screen triangles */
    if (__OBZ_triangle_offscreen(p0, p1, p2, ctx->width, ctx->height))
      continue;

    // if (__OBZ_backface_cull(verts_world[i0], verts_world[i1], verts_world[i2], cam_fwd))
    //   continue;

    float dx1 = p1.x - p0.x;
    float dy1 = p1.y - p0.y;
    float dx2 = p2.x - p0.x;
    float dy2 = p2.y - p0.y;

    if ((dx1 * dy2 - dy1 * dx2) <= 0.0f)
      continue;

    int mid = mesh->face_mtl_id ? mesh->face_mtl_id[t] : -1;

    if (mid != current_material_id)
    {
      current_material_id = mid;

      if (mid >= 0 && mid < mesh->material_count)
      {
        OBZ_ObjMtl* M     = &mesh->materials[mid];
        current_tex       = M->map_Kd ? M->map_Kd : &fallback_tex;
        current_diffuse.r = __OBZ_clampf_to_channel(M->Kd.x);
        current_diffuse.g = __OBZ_clampf_to_channel(M->Kd.y);
        current_diffuse.b = __OBZ_clampf_to_channel(M->Kd.z);
        current_diffuse.a = 255;
      }
      else
      {
        current_tex     = &fallback_tex;
        current_diffuse = COLOR_WHITE;
      }
    }

    /* UVs (per-triangle) */
    static Vec2 uv0 = {0, 0}, uv1 = {0, 0}, uv2 = {0, 0};
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

    /* Pass view-space 1/z (precomputed per-vertex) to rasterizer */
    __OBZ_render_triangle(ctx, p0, p1, p2, uv0, uv1, uv2, one_by_z[i0], one_by_z[i1], one_by_z[i2],
                          current_tex, current_diffuse, 0);
  }
}
