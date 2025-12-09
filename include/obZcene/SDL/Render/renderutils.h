#ifndef OBZ_SDL_RENDER_UTILS_H
#define OBZ_SDL_RENDER_UTILS_H

#include "Math/vec.h"
#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
#include <stdbool.h>

// internal render utils not exposed in public API

OBZ_BEGIN_CPP_DECLS

OBZ_INTERNAL_INLINE static double __OBZ_edge_func_d(const Vec3d* a, const Vec3d* b, double px,
                                                    double py)
{
  return (double)(b->x - a->x) * (py - (double)a->y) - (double)(b->y - a->y) * (px - (double)a->x);
}

OBZ_INTERNAL_INLINE static double __OBZ_edge_func_f(const Vec3f* a, const Vec3f* b, float px,
                                                    float py)
{
  return (float)(b->x - a->x) * (py - (float)a->y) - (float)(b->y - a->y) * (px - (float)a->x);
}

OBZ_INTERNAL_INLINE static int __OBZ_is_top_left_edge(const Vec3f* a, const Vec3f* b)
{
  /* top-left rule: edge is top-left if it is exactly horizontal and b.x > a.x,
     or if b.y < a.y (higher on screen since y grows downward in screen coords). */
  if (b->y < a->y)
    return 1;
  if (b->y == a->y && b->x > a->x)
    return 1;
  return 0;
}

OBZ_INTERNAL void  __OBZ_barycentric_persp(Vec3f v0, Vec3f v1, Vec3f v2, int x, int y, float* w0,
                                           float* w1, float* w2);
OBZ_INTERNAL Vec2f __OBZ_interp_uv_persp(Vec2f uv0, Vec2f uv1, Vec2f uv2, float one_by_z0,
                                         float one_by_z1, float one_by_z2, float w0, float w1,
                                         float w2);
OBZ_INTERNAL bool  __OBZ_triangle_offscreen(Vec3f p0, Vec3f p1, Vec3f p2, int W, int H);
OBZ_INTERNAL OBZ_pixel __OBZ_sample_texture(const OBZ_Texture* tex, Vec2f uv, OBZ_Color diffuse);
OBZ_INTERNAL void      __OBZ_render_clear_depth_buffer(OBZ_DynArray* zbuf, const int n,
                                                       const float* zbuf_clear_ptr);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_RENDER_UTILS_H */
