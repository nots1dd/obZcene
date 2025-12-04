#include "SDL/Camera/camera.h"
#include "Math/clamp.h"
#include <math.h>

OBZ_Camera obz_camera_init(const Vec3 pos, float W, float H, float fov_deg)
{
  float pitch = 0.0f;
  float yaw   = 0.0f;

  Vec3 dir = obz_vec3(0, 0, -1);

  OBZ_Camera cam = {.position  = pos,
                    .direction = dir,
                    .velocity  = obz_vec3(0, 0, 0),

                    .pitch = pitch,
                    .yaw   = yaw,
                    .roll  = 0.0f,

                    .fov    = fov_deg * M_PI / 180.0f,
                    .aspect = W / H,
                    .near   = 0.1f,
                    .far    = 5 * W};

  return cam;
}

void obz_camera_update(OBZ_Camera* cam, float dt)
{
  cam->position.x += cam->velocity.x * dt;
  cam->position.y += cam->velocity.y * dt;
  cam->position.z += cam->velocity.z * dt;

  if (cam->has_bounds)
  {
    cam->position.x = obz_clampf(cam->position.x, cam->min_x, cam->max_x);
    cam->position.y = obz_clampf(cam->position.y, cam->min_y, cam->max_y);
    cam->position.z = obz_clampf(cam->position.z, cam->min_z, cam->max_z);
  }
}

void obz_camera_set_bounds(OBZ_Camera* cam, float min_x, float max_x, float min_y, float max_y,
                           float min_z, float max_z)
{
  cam->has_bounds = true;
  cam->min_x      = min_x;
  cam->max_x      = max_x;
  cam->min_y      = min_y;
  cam->max_y      = max_y;
  cam->min_z      = min_z;
  cam->max_z      = max_z;
}

void obz_camera_update_direction(OBZ_Camera* cam)
{
  float cp = cosf(cam->pitch);
  float sp = sinf(cam->pitch);
  float cy = cosf(cam->yaw);
  float sy = sinf(cam->yaw);

  cam->direction.x = cp * sy;
  cam->direction.y = sp;
  cam->direction.z = -cp * cy;
  cam->direction   = obz_vec3_norm(cam->direction);
}

// Projects a world-space point into pixel coords.
// Returns 0 on success, 1 if behind near/far or degenerate.
OBZ_CameraStatus obz_project_camera(const Vec3 world_pos, const OBZ_Camera cam, int* px, int* py,
                                    int sw, int sh)
{
  if (!px || !py)
    return OBZ_CAMERA_OUT_OF_BOUNDS;

  // Camera basis
  Vec3       fwd      = obz_vec3_norm(cam.direction);
  const Vec3 world_up = {0, 1, 0};

  Vec3 right = obz_vec3_cross(world_up, fwd);
  if (obz_vec3_len(right) < 1e-6f)
  {
    right = obz_vec3_cross((Vec3){0, 0, 1}, fwd);
  }
  right = obz_vec3_norm(right);

  Vec3 up = obz_vec3_cross(fwd, right);

  // Transform into camera space
  Vec3  d = obz_vec3_sub(world_pos, cam.position);
  float x = obz_vec3_dot(d, right);
  float y = obz_vec3_dot(d, up);
  float z = obz_vec3_dot(d, fwd);

  // Reject behind near/far
  if (z <= cam.near || z >= cam.far)
    return OBZ_CAMERA_OUT_OF_BOUNDS;

  // Projection
  float f = 1.0f / tanf(cam.fov * 0.5f);

  float ndc_x = (x * f) / (z * cam.aspect);
  float ndc_y = (y * f) / z;

  // Map NDC -> screen pixels
  *px = (int)(sw * 0.5f + ndc_x * sw * 0.5f);
  *py = (int)(sh * 0.5f - ndc_y * sh * 0.5f);

  return OBZ_CAMERA_OK;
}
