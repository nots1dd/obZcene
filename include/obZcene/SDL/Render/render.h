#ifndef OBZ_RENDER_H
#define OBZ_RENDER_H

#include "SDL/Camera/camera.h"
#include "SDL/Colors/colors.h"
#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
#include "types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct OBZ_RendererContext
  {
    OBZ_pixel*   framebuffer; // RGBA framebuffer
    OBZ_DynArray zbuffer;     // depth buffer
    int          width;
    int          height;
    OBZ_Camera*  cam;
  } OBZ_RendererContext;

  // !!AGBR scheme!!
  OBZ_FORCE_INLINE static OBZ_pixel __OBZ_pack_color(OBZ_Color c)
  {
    return (c.a << 24) | (c.b << 16) | (c.g << 8) | c.r;
  }

  inline static void obz_renderer_init_zbuffer(OBZ_RendererContext* ctx)
  {
    obz_arr_reserve(&ctx->zbuffer, ctx->width * ctx->height);
    const float clearbuf = 1.0f;
    for (int i = 0; i < ctx->width * ctx->height; i++)
      obz_arr_push(&ctx->zbuffer, &clearbuf); // farthest depth
  }

  // Clear framebuffer + depth buffer
  OBZ_API void obz_render_clear(OBZ_RendererContext* ctx, OBZ_pixel clear_color);
  OBZ_API void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color);
  OBZ_API void obz_render_triangle(OBZ_RendererContext* ctx, Vec3 v0, Vec3 v1, Vec3 v2, Vec2 uv0,
                                   Vec2 uv1, Vec2 uv2, OBZ_Texture* tex);
  OBZ_API void obz_render_mesh_textured_camera(OBZ_RendererContext* ctx, Vec3 pos, OBZ_Mesh3D* mesh,
                                             OBZ_MeshTextures* texs, float pitch, float yaw,
                                             float roll, OBZ_Camera cam);

#ifdef __cplusplus
}
#endif

#endif /* OBZ_RENDER_H */
