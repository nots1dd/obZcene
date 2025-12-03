#ifndef OBZ_SDL_RENDER_H
#define OBZ_SDL_RENDER_H

#include "SDL/Camera/camera.h"
#include "SDL/Colors/colors.h"
#include "SDL/Mesh/mesh.h"
#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
#include "obz_types.h"
#include <stdint.h>

#define OBZ_Z_BUF_CLEAR 1.0f

OBZ_BEGIN_CPP_DECLS

typedef struct
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

OBZ_FORCE_INLINE static OBZ_pixel __OBZ_pack_color_channels(OBZ_channel r, OBZ_channel g,
                                                            OBZ_channel b, OBZ_channel a)
{
  return (a << 24) | (b << 16) | (g << 8) | r;
}

OBZ_API OBZ_RendererContext* obz_render_context_init(int width, int height);
OBZ_API void                 obz_render_clear(OBZ_RendererContext* ctx, OBZ_pixel clear_color);
OBZ_API void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color);
OBZ_API void obz_render_triangle(OBZ_RendererContext* ctx, Vec3 v0, Vec3 v1, Vec3 v2, Vec2 uv0,
                                 Vec2 uv1, Vec2 uv2, OBZ_Texture* tex);
OBZ_API void obz_render_mesh_camera(OBZ_RendererContext* ctx, Vec3 pos, OBZ_Mesh3D* mesh,
                                    float pitch, float yaw, float roll, OBZ_Camera cam);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_RENDER_H */
