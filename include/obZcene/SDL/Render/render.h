#ifndef OBZ_RENDER_H
#define OBZ_RENDER_H

#include "SDL/Camera/camera.h"
#include "SDL/Colors/colors.h"
#include "SDL/Textures/textures.h"
#include "Utils/vec.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct OBZ_RendererContext
  {
    pixel*      framebuffer; // RGBA framebuffer
    int         width;
    int         height;
    OBZ_Camera* cam;
  } OBZ_RendererContext;

  typedef struct
  {
    int x, y;
  } OBZ_Point;

  /* ==========================================================
   Frame Control
   ========================================================== */
  OBZ_API void obz_renderer_clear(OBZ_RendererContext* ctx, pixel r, pixel g, pixel b, pixel a);

  /* ==========================================================
   Primitive Drawing
   ========================================================== */
  OBZ_API void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, OBZ_Color color);
  OBZ_API void obz_draw_line(OBZ_RendererContext* ctx, Vec2 p0, Vec2 p1, OBZ_Color color);
  OBZ_API void obz_draw_mesh_textured_camera(OBZ_RendererContext* ctx, Vec3 pos, OBZ_Mesh3D* mesh,
                                             OBZ_MeshTextures* textures, OBZ_Color color,
                                             float pitch, float yaw, float roll, OBZ_Camera cam);

#ifdef __cplusplus
}
#endif

#endif /* OBZ_RENDER_H */
