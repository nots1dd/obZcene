#ifndef OBZ_SDL_CAMERA_H
#define OBZ_SDL_CAMERA_H

#include "Math/simd/x86/trig_pack.h"
#include "Utils/vec.h"
#include <stdbool.h>

OBZ_BEGIN_CPP_DECLS

typedef enum
{
  OBZ_CAMERA_OK = 0,
  OBZ_CAMERA_OUT_OF_BOUNDS
} OBZ_CameraStatus;

typedef struct
{
  Vec3 position;  // Camera position
  Vec3 direction; // Normalized direction vector (camera forward)
  Vec3 velocity;  // Forward velocity (units per second or per step)

  float pitch; // rotation around X axis
  float yaw;   // rotation around Y axis
  float roll;  // rotation around Z axis

  float aspect;

  float fov;
  float near;
  float far;

  float min_x, max_x;
  float min_y, max_y;
  float min_z, max_z;

  bool has_bounds;
} OBZ_Camera;

// Initialize camera with sensible defaults
OBZ_API OBZ_Camera obz_camera_init(const Vec3 pos, float W, float H, float fov_deg);
OBZ_API void       obz_camera_update(OBZ_Camera* cam, float dt);
OBZ_API void       obz_camera_set_bounds(OBZ_Camera* cam, float min_x, float max_x, float min_y,
                                         float max_y, float min_z, float max_z);
OBZ_API void       obz_camera_update_direction(OBZ_Camera* cam);
// projection with FOV handling
OBZ_API OBZ_CameraStatus obz_project_camera(Vec3 world_pos, OBZ_Camera cam, int* px, int* py,
                                            int sw, int sh);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_CAMERA_H */
