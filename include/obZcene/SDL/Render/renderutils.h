#ifndef OBZ_SDL_RENDER_UTILS_H
#define OBZ_SDL_RENDER_UTILS_H

#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
#include "Math/vec.h"
#include <stdbool.h>

// internal render utils not exposed in public API

OBZ_BEGIN_CPP_DECLS

OBZ_FORCE_INLINE static double __OBZ_edge_func_d(const Vec3d* a, const Vec3d* b, double px,
                                                 double py)
{
  return (double)(b->x - a->x) * (py - (double)a->y) - (double)(b->y - a->y) * (px - (double)a->x);
}

OBZ_FORCE_INLINE static int __OBZ_is_top_left_edge(const Vec3d* a, const Vec3d* b)
{
  /* top-left rule: edge is top-left if it is exactly horizontal and b.x > a.x,
     or if b.y < a.y (higher on screen since y grows downward in screen coords). */
  if (b->y < a->y)
    return 1;
  if (b->y == a->y && b->x > a->x)
    return 1;
  return 0;
}

OBZ_INTERNAL_API void      __OBZ_barycentric_persp(Vec3d v0, Vec3d v1, Vec3d v2, int x, int y,
                                                   double* w0, double* w1, double* w2);
OBZ_INTERNAL_API Vec2d     __OBZ_interp_uv_persp(Vec2d uv0, Vec2d uv1, Vec2d uv2, double one_by_z0,
                                                 double one_by_z1, double one_by_z2, double w0,
                                                 double w1, double w2);
OBZ_INTERNAL_API bool      __OBZ_triangle_offscreen(Vec3d p0, Vec3d p1, Vec3d p2, int W, int H);
OBZ_INTERNAL_API OBZ_pixel __OBZ_sample_texture(const OBZ_Texture* tex, Vec2d uv,
                                                OBZ_Color diffuse);
OBZ_INTERNAL_API void      __OBZ_render_clear_depth_buffer(OBZ_DynArray* zbuf, const int n,
                                                           const double* zbuf_clear_ptr);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_RENDER_UTILS_H */
