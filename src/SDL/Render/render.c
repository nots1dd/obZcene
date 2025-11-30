#include "SDL/Render/render.h"
#include "obz_log.h"
#include <alloca.h>
#include <math.h>

OBZ_RendererContext* obz_render_context_init(int width, int height)
{
  OBZ_RendererContext* renctx = malloc(sizeof(*renctx));
  if (!renctx)
  {
    OBZ_LOG_ERROR(NULL, "Failed to allocate RendererContext.");
    return NULL;
  }

  renctx->width  = width;
  renctx->height = height;

  const size_t pixel_count = (size_t)(width * height);

  renctx->framebuffer = malloc(pixel_count * 4); // RGBA colors (!!ABGR FORMAT!!)
  if (!renctx->framebuffer)
  {
    OBZ_LOG_ERROR(NULL, "Failed to allocate framebuffer.");
    free(renctx);
    return NULL;
  }

  // Float Z-buffer
  renctx->zbuffer = obz_arr_create(sizeof(float));
  obz_arr_reserve(&renctx->zbuffer, pixel_count);

  OBZ_ASSERT(obz_arr_exists(&renctx->zbuffer));
  OBZ_ASSERT(renctx != NULL);

  OBZ_LOG_INFO(NULL, "obZcene renderer context created successfully (w: %d, h: %d).", width,
               height);

  return renctx;
}

void obz_render_clear(OBZ_RendererContext* ctx, OBZ_pixel clear_color)
{
  int n = ctx->width * ctx->height;

  // Clear framebuffer
  for (int i = 0; i < n; i++)
    ctx->framebuffer[i] = clear_color;

  // Clear z-buffer if it exists
  if (obz_arr_exists(&ctx->zbuffer))
  {
    const float z_clear = OBZ_Z_BUF_CLEAR;
    for (int i = 0; i < n; i++)
      obz_arr_set(&ctx->zbuffer, i, &z_clear);
  }
}

// Put pixel with depth test
void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color)
{
  if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
    return;

  int idx = y * ctx->width + x;

  if (obz_arr_exists(&ctx->zbuffer))
  {
    float* p_z = (float*)obz_arr_get(&ctx->zbuffer, idx);
    if (!p_z)
      return;

    if (z > *p_z)
      return;

    obz_arr_set(&ctx->zbuffer, idx, &z);
  }

  ctx->framebuffer[idx] = color;
}

void obz_render_triangle(OBZ_RendererContext* ctx, Vec3 v0, Vec3 v1, Vec3 v2, Vec2 uv0, Vec2 uv1,
                         Vec2 uv2, OBZ_Texture* tex)
{
  if (!ctx)
    return;

  int minx = (int)floorf(fminf(fminf(v0.x, v1.x), v2.x));
  int maxx = (int)ceilf(fmaxf(fmaxf(v0.x, v1.x), v2.x));
  int miny = (int)floorf(fminf(fminf(v0.y, v1.y), v2.y));
  int maxy = (int)ceilf(fmaxf(fmaxf(v0.y, v1.y), v2.y));

  if (minx < 0)
    minx = 0;
  if (miny < 0)
    miny = 0;
  if (maxx >= ctx->width)
    maxx = ctx->width - 1;
  if (maxy >= ctx->height)
    maxy = ctx->height - 1;
  if (minx > maxx || miny > maxy)
    return;

  const float EPS   = 1e-6f;
  float       denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
  if (fabsf(denom) < EPS)
    return; // degenerate

  const bool zbuf_exists = obz_arr_exists(&ctx->zbuffer);

  for (int y = miny; y <= maxy; ++y)
  {
    for (int x = minx; x <= maxx; ++x)
    {
      float w0 = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / denom;
      float w1 = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / denom;
      float w2 = 1.0f - w0 - w1;

      if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
        continue;

      // interpolate depth
      float z     = w0 * v0.z + w1 * v1.z + w2 * v2.z;
      int   slice = y * ctx->width + x;

      if (zbuf_exists)
      {
        float* zele = (float*)obz_arr_get(&ctx->zbuffer, (size_t)slice);
        if (!zele)
          continue;
        if (z > *zele)
          continue;
        obz_arr_set(&ctx->zbuffer, (size_t)slice, &z);
      }

      OBZ_pixel color_val = __OBZ_pack_color(COLOR_WHITE); // default

      if (tex && tex->pixels && tex->width > 0 && tex->height > 0)
      {
        // interpolate UV (normalized 0..1)
        float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
        float v = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;

        // scale to texel coords and clamp
        int tx = (int)floorf(fminf(fmaxf(u * (tex->width - 1), 0.0f), tex->width - 1));
        int ty = (int)floorf(fminf(fmaxf(v * (tex->height - 1), 0.0f), tex->height - 1));

        OBZ_channel* p = (OBZ_channel*)tex->pixels + 4 * (ty * tex->width + tx);
        color_val = (((OBZ_pixel)p[0] << 24) | ((OBZ_pixel)p[3] << 16) | ((OBZ_pixel)p[2] << 8) |
                     ((OBZ_pixel)p[1]));
      }

      ctx->framebuffer[slice] = color_val;
    }
  }
}

void __OBZ_render_quad(OBZ_RendererContext* ctx, Vec3 v[4], Vec2 uv[4], OBZ_Texture* tex)
{
  obz_render_triangle(ctx, v[0], v[1], v[2], uv[0], uv[1], uv[2], tex);
  obz_render_triangle(ctx, v[0], v[2], v[3], uv[0], uv[2], uv[3], tex);
}

void obz_render_mesh_textured_camera(OBZ_RendererContext* ctx, Vec3 pos, OBZ_Mesh3D* mesh,
                                     OBZ_MeshTextures* texs, float pitch, float yaw, float roll,
                                     OBZ_Camera cam)
{
  if (!ctx || !mesh)
    return;

  int nverts = mesh->nverts;
  int nfaces = mesh->nfaces;
  if (nverts <= 0 || nfaces <= 0)
    return;

  // Allocate transformed vertices
  Vec3* verts_world  = alloca(sizeof(Vec3) * nverts);
  Vec3* verts_screen = alloca(sizeof(Vec3) * nverts);

  // Precompute rotation terms (renamed to avoid name clashes)
  float cp = cosf(pitch), sp = sinf(pitch);
  float cy = cosf(yaw), syaw = sinf(yaw);
  float cr = cosf(roll), sroll = sinf(roll);

  // Transform all vertices
  for (int i = 0; i < nverts; i++)
  {
    Vec3 v = mesh->verts[i];

    float x1 = v.x * cy - v.z * syaw;
    float z1 = v.x * syaw + v.z * cy;
    float y2 = v.y * cp - z1 * sp;
    float z2 = v.y * sp + z1 * cp;
    float x3 = x1 * cr - y2 * sroll;
    float y3 = x1 * sroll + y2 * cr;

    verts_world[i] = (Vec3){x3 + pos.x, y3 + pos.y, z2 + pos.z};

    int pxi, pyi;
    obz_project_camera(verts_world[i], cam, &pxi, &pyi, ctx->width, ctx->height);
    verts_screen[i] = (Vec3){(float)pxi, (float)pyi, verts_world[i].z};
  }

  // Prepare a neutral white fallback 1x1 texture (used only when no texture is bound)
  static OBZ_Texture fallback_tex;
  static OBZ_channel fallback_pixel[4] = {255, 255, 255, 255}; // RGBA white
  fallback_tex.id                      = 0;
  fallback_tex.width                   = 1;
  fallback_tex.height                  = 1;
  fallback_tex.pixels                  = fallback_pixel;

  // Draw each face
  for (int f = 0; f < nfaces; f++)
  {
    int* face = mesh->faces[f];
    if (!face)
      continue;

    // Validate indices
    int valid_face = 1;
    for (int i = 0; i < 4; i++)
      if (face[i] < 0 || face[i] >= nverts)
        valid_face = 0;
    if (!valid_face)
      continue;

    // Copy screen-space verts and simple visibility test
    Vec3 quad_verts[4];
    Vec2 quad_uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    int  visible     = 1;
    for (int i = 0; i < 4; i++)
    {
      quad_verts[i] = verts_screen[face[i]];
      if (quad_verts[i].x < 0 || quad_verts[i].y < 0)
        visible = 0;
    }
    if (!visible)
      continue;

    // Pick texture: consult texs->face_tex_index (if present) then textures[]
    OBZ_Texture* tex = &fallback_tex; // default
    if (texs)
    {
      int slot = 0;
      if (f < OBZ_MAX_MESH_FACES)
        slot = texs->face_tex_index[f]; // your API; ensure initialization elsewhere
      if (slot >= 0 && slot < OBZ_MAX_MESH_TEXTURES && texs->textures[slot])
        tex = texs->textures[slot];
    }

    __OBZ_render_quad(ctx, quad_verts, quad_uvs, tex);
  }
}
