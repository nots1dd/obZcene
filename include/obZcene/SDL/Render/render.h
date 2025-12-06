#ifndef OBZ_SDL_RENDER_H
#define OBZ_SDL_RENDER_H

#include "SDL/Camera/camera.h"
#include "SDL/Mesh/mesh.h"
#include "Utils/arena_allocator.h"
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
  OBZ_pixel    background_color;

  OBZ_Arena* arena_alloc;

} OBZ_RendererContext;

OBZ_API OBZ_RendererContext* obz_render_context_init(int width, int height);
OBZ_API void                 obz_render_clear(OBZ_RendererContext* ctx, OBZ_pixel clear_color);
OBZ_API void obz_render_pixel(OBZ_RendererContext* ctx, int x, int y, float z, OBZ_pixel color);
OBZ_API void obz_render_mesh_camera(OBZ_RendererContext* ctx, Vec3 pos, OBZ_Mesh3D* mesh,
                                    float pitch, float yaw, float roll, OBZ_Camera cam);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_RENDER_H */
