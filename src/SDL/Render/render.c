#include "SDL/Render/render.h"
#include "Math/clamp.h"
#include "Math/minimax.h"
#include "SDL/Render/renderutils.h"
#include "Utils/arena_allocator.h"
#include "Utils/null.h"
#include "obz_log.h"
#include "obz_mem.h"

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

  renctx->zbuffer = obz_arr_create(sizeof(float));
  obz_arr_reserve(&renctx->zbuffer, pixel_count);

  OBZ_ASSERT(obz_arr_exists(&renctx->zbuffer) == false,
             "Something went wrong allocating z-buffer!");
  OBZ_ASSERT(renctx != NULL, "Renderer context is NULL after allocation!");

  OBZ_LOG_INFO(NULL, "obZcene renderer context created successfully (w: %d, h: %d).", width,
               height);

  return renctx;
}

void obz_render_clear(OBZ_RendererContext* ctx, OBZ_pixel clear_color)
{
  const int n = ctx->width * ctx->height;

  // clear framebuffer
  for (int i = 0; i < n; i++)
    ctx->framebuffer[i] = clear_color;

  // clear zbuf if it exists
  const float z_clear = OBZ_Z_BUF_CLEAR;
  __OBZ_render_clear_depth_buffer(&ctx->zbuffer, n, &z_clear);
}

void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color)
{
  if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
    return;

  int idx = y * ctx->width + x;

  auto zb = obz_arr_exists(&ctx->zbuffer) ? obz_arr_get_data(&ctx->zbuffer, float) : NULL;

  if (zb)
  {
    if (z > zb[idx])
      return;
    zb[idx] = z;
  }

  ctx->framebuffer[idx] = color;
}

// -----------------------------------------------------------------------------
// TRIANGLE RASTERIZATION: SCREEN-SPACE BARYCENTRIC + DEPTH + TEXTURE INTERP
// -----------------------------------------------------------------------------
static void __OBZ_render_triangle(OBZ_RendererContext* ctx, Vec3f v0, Vec3f v1, Vec3f v2, Vec2f uv0,
                                  Vec2f uv1, Vec2f uv2, float iz0, float iz1, float iz2,
                                  OBZ_Texture* tex, OBZ_Color diffuse, int use_color_only)
{
  __OBZ_RETURN_VOID_IF_NULL(ctx);

  // ----------------------- SCREEN-SPACE BOUNDING BOX -------------------------
  /*
      Compute axis-aligned bounding box of the projected triangle.

      We rasterize only pixels inside:
          x ∈ [min(v0.x, v1.x, v2.x), max(...)]
          y ∈ [min(v0.y, v1.y, v2.y), max(...)]

      Clamped to screen resolution.
      Reduces fill cost dramatically vs scanning entire framebuffer.
  */
  // ----------------------------------------------------------------------------
  int minx = obz_clamp_floor_to_int(obz_minf(obz_minf(v0.x, v1.x), v2.x), 0, ctx->width - 1);
  int maxx = obz_clamp_ceil_to_int(obz_maxf(obz_maxf(v0.x, v1.x), v2.x), 0, ctx->width - 1);
  int miny = obz_clamp_floor_to_int(obz_minf(obz_minf(v0.y, v1.y), v2.y), 0, ctx->height - 1);
  int maxy = obz_clamp_ceil_to_int(obz_maxf(obz_maxf(v0.y, v1.y), v2.y), 0, ctx->height - 1);

  if (minx > maxx || miny > maxy)
    return;

  // Depth buffer pointer once (avoid repeated calls)
  auto zb = obz_arr_exists(&ctx->zbuffer) ? obz_arr_get_data(&ctx->zbuffer, float) : NULL;

  // ------------------------ SOLID COLOR PRE-COMPUTE --------------------------
  /*
      If no texture is present or "color only" mode is used,
      pre-pack diffuse color once.

      Avoids per-pixel packing cost.
  */
  // ---------------------------------------------------------------------------
  const OBZ_pixel solid_color_pixels =
    (use_color_only || !tex || !tex->pixels) ? __OBZ_pack_color(diffuse) : 0;

  // --------------------------- RASTERIZATION LOOP ----------------------------
  /*
      For each pixel inside bounding box:
        1. Compute barycentric coords (w0,w1,w2)
        2. Reject if outside triangle (any weight < 0)
        3. Interpolate depth z = Σ(w_i * v_i.z)
        4. Depth test
        5. If textured: perspective-correct UV interpolation
        6. Write final pixel
  */
  // ---------------------------------------------------------------------------
  for (int y = miny; y <= maxy; ++y)
  {
    for (int x = minx; x <= maxx; ++x)
    {

      // --------------------- BARYCENTRIC (EDGE FUNCTIONS) ---------------------
      /*
          Barycentric weights from 2D screen positions:
              w0 = area(P, v1, v2) / area(v0,v1,v2)
              w1 = area(v0, P, v2) / area(v0,v1,v2)
              w2 = 1 - w0 - w1

          If any weight < 0 ⇒ pixel outside triangle.
      */
      // ------------------------------------------------------------------------
      Vec3f bary_persp = __OBZ_barycentric_persp(v0, v1, v2, x, y);

      float w0 = bary_persp.x, w1 = bary_persp.y, w2 = bary_persp.z;

      if (w0 < 0 || w1 < 0 || w2 < 0)
        continue;

      // -------------------------- DEPTH INTERPOLATION -------------------------
      /*
          True view-space depth (vz):
              z = Σ(w_i * v_i.z)

          Used for depth test.
      */
      // ------------------------------------------------------------------------
      float z   = w0 * v0.z + w1 * v1.z + w2 * v2.z;
      int   idx = y * ctx->width + x;

      // ------------------------------- DEPTH TEST -----------------------------
      /*
          Smaller z = closer.

          Only sample texture after passing depth test
          (avoids wasteful sampling).
      */
      // ------------------------------------------------------------------------
      if (zb)
      {
        if (z > zb[idx])
          continue;
        zb[idx] = z;
      }

      OBZ_pixel out_buffer;

      // --------------------------- TEXTURE OR SOLID ---------------------------
      if (use_color_only || !tex || !tex->pixels)
      {
        out_buffer = solid_color_pixels;
      }
      else
      {
        // ------------------ PERSPECTIVE-CORRECT UV INTERP --------------------
        /*
            Interpolate uv / z and 1/z, then divide:

                u = Σ(w_i * (u_i / z_i))
                v = Σ(w_i * (v_i / z_i))
                iz = 1 / Σ(w_i * (1 / z_i))

            Final:
                uv_corrected = (u * iz, v * iz)

            This fixes distortion seen with linear UV interpolation.
        */
        // ----------------------------------------------------------------------
        Vec2f uv   = __OBZ_interp_uv_persp(uv0, uv1, uv2, iz0, iz1, iz2, w0, w1, w2);
        out_buffer = __OBZ_sample_texture(tex, uv, diffuse);
      }

      ctx->framebuffer[idx] = out_buffer;
    }
  }
}

// -----------------------------------------------------------------------------
// MESH RENDERING WITH CAMERA VIEW/PROJECTION
// Transforms mesh into camera space → projects → rasterizes each triangle.
// -----------------------------------------------------------------------------
void obz_render_mesh_camera(OBZ_RendererContext* ctx, Vec3f pos, OBZ_Mesh3D* mesh,
                            OBZ_CamRotation mesh_rot, OBZ_Camera cam)
{
  __OBZ_RETURN_VOID_IF_NULL(ctx);
  __OBZ_RETURN_VOID_IF_NULL(mesh);

  const int nverts = mesh->noOfVerts;
  const int nfaces = mesh->noOfFaces;
  if (nverts == 0 || nfaces == 0)
    return;

  // ------------------------- TEMP PER-VERTEX BUFFERS -------------------------
  /*
      verts_world  : position after object → world transform
      verts_screen : projected 2D pixel pos, z = camera-space depth
      one_by_z     : reciprocals of depth for perspective-correct interpolation
  */
  // --------------------------------------------------------------------------
  auto verts_world =
    (Vec3f*)obz_arena_alloc(ctx->arena_alloc, nverts * sizeof(Vec3f), OBZ_ARENA_DEFAULT_ALIGNMENT);
  auto verts_screen =
    (Vec3f*)obz_arena_alloc(ctx->arena_alloc, nverts * sizeof(Vec3f), OBZ_ARENA_DEFAULT_ALIGNMENT);
  auto one_by_z = (float*)obz_arena_alloc(ctx->arena_alloc, nverts * sizeof(float), sizeof(float));

  // -------------------------- OBJECT ROTATION (XYZ) --------------------------
  /*
      Apply yaw (Y), pitch (X), roll (Z) to vertex.

      Using classic intrinsic rotations:
          yaw:   rotate around Y
          pitch: rotate around X
          roll:  rotate around Z

      Produces full orientation of object in world space.
  */
  // --------------------------------------------------------------------------
  float cp = cosf(mesh_rot.pitch), sp = sinf(mesh_rot.pitch);
  float cy = cosf(mesh_rot.yaw), sy = sinf(mesh_rot.yaw);
  float cr = cosf(mesh_rot.roll), sr = sinf(mesh_rot.roll);

  // Camera forward direction (normalized)
  Vec3f cam_fwd = obz_vec3f_norm(cam.direction);

  // ------------------ TRANSFORM VERTICES → WORLD → CAMERA → SCREEN -----------
  /*
      Steps for each vertex:

      1. Apply object rotation + translation → world position
      2. Compute vector from cam to vertex
      3. Depth = dot(view_vec, cam_forward)
      4. Project to screen:
             obz_project_camera(world, cam) → pixel (sx, sy)
      5. Store:
             verts_screen[i] = {sx, sy, depth}
             one_by_z = 1 / depth
  */
  // --------------------------------------------------------------------------
  for (int i = 0; i < nverts; i++)
  {
    Vec3f v = mesh->verts[i];

    // Rotate: yaw → pitch → roll
    float x = v.x, y = v.y, z = v.z;
    float world_x = x * cy * cr - y * sr - z * sy * cr + pos.x;
    float world_y = x * cy * sr + y * cr - z * sy * sr + pos.y;
    float world_z = x * sy + y * sp + z * cy * cp + pos.z;

    Vec3f world    = {world_x, world_y, world_z};
    verts_world[i] = world;

    // Camera depth = projection on camera forward vector
    Vec3f view_vec = obz_vec3_sub(world, cam.position);
    float vz       = obz_vec3f_dot(view_vec, cam_fwd);

    // Screen projection
    int sx = 0, sy = 0;
    if (obz_project_camera(world, cam, &sx, &sy, ctx->width, ctx->height) == OBZ_ERR_CAMERA)
      continue;

    verts_screen[i] = (Vec3f){(float)sx, (float)sy, vz};
    one_by_z[i]     = (vz > 0.0f ? 1.0f / vz : 0.0f);
  }

  // ---------------------------- TRIANGLE LOOP -------------------------------
  /*
      For each face:

        - Load vertex indices
        - Reject triangles behind camera or outside screen
        - Backface cull using signed area in screen space
        - Load material/texture
        - Pass per-vertex:
            screen coords
            UVs
            1/z values
        - Rasterize via __OBZ_render_triangle
  */
  // --------------------------------------------------------------------------
  int* vidx = mesh->indices;
  int* uidx = mesh->uv_indices;

  int          current_material_id = -2;
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

    Vec3f p0 = verts_screen[i0];
    Vec3f p1 = verts_screen[i1];
    Vec3f p2 = verts_screen[i2];

    // --------------------- FRUSTUM + SCREEN CLIPPING ------------------------
    /*
        Simple rejection:
          - completely off-screen rectangle
          - any depth <= 0 (behind camera)
    */
    // ------------------------------------------------------------------------
    if (__OBZ_triangle_offscreen(p0, p1, p2, ctx->width, ctx->height))
      continue;

    // idk how this is working but ok
    if (((p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x)) <= 0.0f)
      continue;

    // --------------------------- MATERIAL SELECTION --------------------------
    /*
        Switch material only when ID changes.

        Loads:
            - diffuse color
            - texture map (map_Kd)
    */
    // -------------------------------------------------------------------------
    int mid = mesh->face_mtl_id ? mesh->face_mtl_id[t] : -1;
    if (mid != current_material_id)
    {
      current_material_id = mid;

      if (mid >= 0 && mid < mesh->material_count)
      {
        OBZ_ObjMtl* M = &mesh->materials[mid];
        current_tex   = M->map_Kd ? M->map_Kd : &fallback_tex;

        // convert diffuse [0,1] → [0,255]
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

    // ---------------------------- LOAD UVs ----------------------------------
    /*
        Per-triangle UV loading:
            uv0 = uvs[uvIdx0]
            uv1 = uvs[uvIdx1]
            uv2 = uvs[uvIdx2]
    */
    // ------------------------------------------------------------------------
    static Vec2f uv0 = {0, 0}, uv1 = {0, 0}, uv2 = {0, 0};
    if (mesh->uvs && mesh->uv_indices)
    {
      int ti0 = uidx[t * 3 + 0];
      int ti1 = uidx[t * 3 + 1];
      int ti2 = uidx[t * 3 + 2];

      int nuv = mesh->noOfUVs;
      uv0     = (ti0 < nuv) ? mesh->uvs[ti0] : uv0;
      uv1     = (ti1 < nuv) ? mesh->uvs[ti1] : uv1;
      uv2     = (ti2 < nuv) ? mesh->uvs[ti2] : uv2;
    }

    // ----------------------- RASTERIZE FINAL TRIANGLE -----------------------
    /*
        Pass:
            projected vertices
            UVs
            1/z values
            current texture
            diffuse color

        Rasterizer performs:
            - barycentric fill
            - depth test
            - perspective UV
            - texture sample * diffuse
    */
    // ------------------------------------------------------------------------
    __OBZ_render_triangle(ctx, p0, p1, p2, uv0, uv1, uv2, one_by_z[i0], one_by_z[i1], one_by_z[i2],
                          current_tex, current_diffuse, 0);
  }
}
