#ifndef OBZ_SDL_CAMERA_H
#define OBZ_SDL_CAMERA_H

#include "Math/simd/x86/trig_pack.h"
#include "Math/vec.h"
#include "obz_types.h"
#include <stdbool.h>

OBZ_BEGIN_CPP_DECLS

typedef struct
{
  float min;
  float max;
} OBZ_CamBound;

typedef struct
{
  float pitch; // rotation around X axis
  float yaw;   // rotation around Y axis
  float roll;  // rotation around Z axis
} OBZ_CamRotation;

OBZ_INTERNAL_INLINE static void __OBZ_add_bound(OBZ_CamBound* bound, float min, float max)
{
  bound->min = min;
  bound->max = max;
}

typedef struct
{
  Vec3f position;  // Camera position
  Vec3f direction; // Normalized direction vector (camera forward)
  Vec3f velocity;  // Forward velocity (units per second or per step)

  OBZ_CamRotation rot;

  float aspect;

  float fov;
  float near;
  float far;

  OBZ_CamBound x_bound;
  OBZ_CamBound y_bound;
  OBZ_CamBound z_bound;

  bool has_bounds;
} OBZ_Camera;

static const Vec3f g_camera_world_up   = {0, 1, 0};
static const Vec3f g_camera_world_init = {0, 0, 0};

// Initialize camera with sensible defaults
OBZ_API OBZ_Camera obz_camera_init(const Vec3f pos, float W, float H, float fov_deg);
OBZ_API void       obz_camera_update(OBZ_Camera* cam, float dt);
OBZ_API void       obz_camera_set_bounds(OBZ_Camera* cam, OBZ_CamBound x_axis, OBZ_CamBound y_axis,
                                         OBZ_CamBound z_axis);
OBZ_API void       obz_camera_update_direction(OBZ_Camera* cam);
// projection with FOV handling
OBZ_API OBZ_Result obz_project_camera(Vec3f world_pos, OBZ_Camera cam, int* px, int* py, int sw,
                                      int sh);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_CAMERA_H */
