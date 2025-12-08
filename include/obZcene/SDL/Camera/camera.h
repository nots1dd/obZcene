#ifndef OBZ_SDL_CAMERA_H
#define OBZ_SDL_CAMERA_H

#include "Math/simd/x86/trig_pack.h"
#include "Math/vec.h"
#include <stdbool.h>

OBZ_BEGIN_CPP_DECLS

typedef enum
{
  OBZ_CAMERA_OK = 0,
  OBZ_CAMERA_OUT_OF_BOUNDS
} OBZ_CameraStatus;

typedef struct
{
  double min;
  double max;
} OBZ_CamBound;

OBZ_API_IMPL static void __OBZ_add_bound(OBZ_CamBound* bound, double min, double max)
{
  bound->min = min;
  bound->max = max;
}

typedef struct
{
  Vec3d position;  // Camera position
  Vec3d direction; // Normalized direction vector (camera forward)
  Vec3d velocity;  // Forward velocity (units per second or per step)

  double pitch; // rotation around X axis
  double yaw;   // rotation around Y axis
  double roll;  // rotation around Z axis

  double aspect;

  double fov;
  double near;
  double far;

  OBZ_CamBound x_bound;
  OBZ_CamBound y_bound;
  OBZ_CamBound z_bound;

  bool has_bounds;
} OBZ_Camera;

// Initialize camera with sensible defaults
OBZ_API OBZ_Camera obz_camera_init(const Vec3d pos, double W, double H, double fov_deg);
OBZ_API void       obz_camera_update(OBZ_Camera* cam, double dt);
OBZ_API void       obz_camera_set_bounds(OBZ_Camera* cam, double min_x, double max_x, double min_y,
                                         double max_y, double min_z, double max_z);
OBZ_API void       obz_camera_update_direction(OBZ_Camera* cam);
// projection with FOV handling
OBZ_API OBZ_CameraStatus obz_project_camera(Vec3d world_pos, OBZ_Camera cam, int* px, int* py,
                                            int sw, int sh);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_CAMERA_H */
