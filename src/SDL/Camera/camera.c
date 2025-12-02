#include "SDL/Camera/camera.h"
#include <math.h>

OBZ_Camera obz_camera_init(const Vec3 pos, float W, [[maybe_unused]] float H, float fov_deg)
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

                    .fov = fov_deg * M_PI / 180.0f,
                    //.aspect = W / H,
                    .near = 0.1f,
                    .far  = 5 * W};

  return cam;
}

void obz_camera_update(OBZ_Camera* cam, float dt)
{
  cam->position.x += cam->velocity.x * dt;
  cam->position.y += cam->velocity.y * dt;
  cam->position.z += cam->velocity.z * dt;

  if (cam->has_bounds)
  {
    if (cam->position.x < cam->min_x)
      cam->position.x = cam->min_x;
    if (cam->position.x > cam->max_x)
      cam->position.x = cam->max_x;
    if (cam->position.y < cam->min_y)
      cam->position.y = cam->min_y;
    if (cam->position.y > cam->max_y)
      cam->position.y = cam->max_y;
    if (cam->position.z < cam->min_z)
      cam->position.z = cam->min_z;
    if (cam->position.z > cam->max_z)
      cam->position.z = cam->max_z;
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

void obz_project_camera(Vec3 world_pos, OBZ_Camera cam, int* px, int* py, int sw, int sh)
{
  if (!px || !py)
    return;

  Vec3 fwd = obz_vec3_norm(cam.direction);

  // Prevent degenerate right vector
  const Vec3 world_up = {0, 1, 0};
  Vec3       right    = obz_vec3_cross(world_up, fwd);
  if (obz_vec3_len(right) < 1e-6f) // almost parallel
    right = obz_vec3_cross((Vec3){0, 0, 1}, fwd);
  right = obz_vec3_norm(right);

  Vec3 up = obz_vec3_cross(fwd, right);

  Vec3 d = obz_vec3_sub(world_pos, cam.position);

  // Transform to camera space
  float x = obz_vec3_dot(d, right);
  float y = obz_vec3_dot(d, up);
  float z = obz_vec3_dot(d, fwd);

  if (z <= cam.near || z >= cam.far)
  {
    *px = -1;
    *py = -1;
    return;
  }

  // Perspective projection
  float aspect = (float)sw / (float)sh;
  float f      = 1.0f / tanf(cam.fov * 0.5f);

  *px = (int)(sw * 0.5f + x * f / z * sw * 0.5f / aspect);
  *py = (int)(sh * 0.5f - y * f / z * sh * 0.5f);
}
