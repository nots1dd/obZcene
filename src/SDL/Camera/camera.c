#include "SDL/Camera/camera.h"
#include "Math/clamp.h"
#include "Math/vec.h"
#include <math.h>

OBZ_Camera obz_camera_init(const Vec3f pos, float W, float H, float fov_deg)
{
  // ---------------------- DEG -> RAD + ASPECT RATIO ----------------------
  /*
      FOV (field of view) is commonly specified in degrees, but camera math
      uses radians.

          fov_rad = fov_deg * π / 180

      Aspect ratio is:

          aspect = width / height

      Both are used in the perspective projection matrix.
  */
  // ----------------------------------------------------------------------
  OBZ_Camera cam = {.position  = pos,
                    .direction = obz_vec3f(0, 0, 0),
                    .velocity  = obz_vec3f(0, 0, 0),

                    .rot.pitch = 0.0f,
                    .rot.yaw   = 0.0f,
                    .rot.roll  = 0.0f,

                    .fov    = fov_deg * M_PI / 180.0f,
                    .aspect = W / H,
                    .near   = 0.1f,
                    .far    = 5 * W};

  return cam;
}

void obz_camera_update(OBZ_Camera* cam, float dt)
{
  // ---------------------- POSITION INTEGRATION (Euler) ----------------------
  /*
      Simple Euler integration:

          position += velocity * dt

      No physics, just linear translation based on constant velocity.
  */
  // --------------------------------------------------------------------------
  cam->position.x += cam->velocity.x * dt;
  cam->position.y += cam->velocity.y * dt;
  cam->position.z += cam->velocity.z * dt;

  if (cam->has_bounds)
  {
    // ---------------------- CLAMPING ----------------------
    /*
        Each axis is clamped to bounded intervals:

            x ∈ [min_x, max_x]
            y ∈ [min_y, max_y]
            z ∈ [min_z, max_z]

        Standard clamp: min(max(v, low), high)
    */
    // -------------------------------------------------------
    cam->position.x = obz_clampf(cam->position.x, cam->x_bound.min, cam->x_bound.max);
    cam->position.y = obz_clampf(cam->position.y, cam->y_bound.min, cam->y_bound.max);
    cam->position.z = obz_clampf(cam->position.z, cam->z_bound.min, cam->z_bound.max);
  }
}

void obz_camera_set_bounds(OBZ_Camera* cam, OBZ_CamBound x_axis, OBZ_CamBound y_axis,
                           OBZ_CamBound z_axis)
{
  cam->has_bounds = true;
  __OBZ_add_bound(&cam->x_bound, x_axis.min, x_axis.max);
  __OBZ_add_bound(&cam->y_bound, y_axis.min, y_axis.max);
  __OBZ_add_bound(&cam->z_bound, z_axis.min, z_axis.max);
}

void obz_camera_update_direction(OBZ_Camera* cam)
{
  // ---------------------- SPHERICAL COORDINATE ROTATION ----------------------
  /*
      Camera direction is derived from yaw (horizontal) and pitch (vertical).

      Using spherical coordinates:

          x = cos(pitch) * sin(yaw)
          y = sin(pitch)
          z = -cos(pitch) * cos(yaw)

      These produce a unit vector on the sphere, rotated by the angles.
      This is equivalent to constructing a forward direction vector from
      Euler angles. The resulting direction is normalized to ensure
      stable unit length.
  */
  // ---------------------------------------------------------------------------
  float cp = cos(cam->rot.pitch);
  float sp = sin(cam->rot.pitch);
  float cy = cos(cam->rot.yaw);
  float sy = sin(cam->rot.yaw);

  cam->direction.x = cp * sy;
  cam->direction.y = sp;
  cam->direction.z = -cp * cy;
  cam->direction   = obz_vec3f_norm(cam->direction);
}

// Projects a world-space point into pixel coords.
// Returns 0 on success, 1 if behind near/far or degenerate.
OBZ_Result obz_project_camera(const Vec3f world_pos, const OBZ_Camera cam, int* px, int* py, int sw,
                              int sh)
{
  if (!px || !py)
    return OBZ_ERR_CAMERA;

  // ---------------------- CAMERA BASIS (Right/Up/Forward) ----------------------
  /*
      We construct an orthonormal basis from the forward direction:

          fwd = normalize(direction)

      The global up vector (0,1,0) is used to derive the camera's right axis:

          right = normalize(up × fwd)

      And then:

          up = fwd × right

      This yields a right-handed coordinate system:

          (right, up, fwd)

      These vectors form the rotation component of the view matrix.
  */
  // -------------------------------------------------------------------------------
  Vec3f fwd = obz_vec3f_norm(cam.direction);

  Vec3f right = obz_vec3f_cross(g_camera_world_up, fwd);
  if (obz_vec3f_len(right) < 1e-6f)
  {
    // Degenerate case: camera looking straight up/down.
    right = obz_vec3f_cross((Vec3f){0, 0, 1}, fwd);
  }
  right = obz_vec3f_norm(right);

  Vec3f up = obz_vec3f_cross(fwd, right);

  // ---------------------- TRANSFORM: WORLD -> CAMERA SPACE ------------------------
  /*
      Camera space coordinates are computed by projecting the offset vector
      onto the camera basis:

          d = world_pos - camera_pos

          x_cam = dot(d, right)
          y_cam = dot(d, up)
          z_cam = dot(d, fwd)

      This effectively applies the inverse of the rotation part of the view matrix.
  */
  // --------------------------------------------------------------------------------
  Vec3f d = obz_vec3f_sub(world_pos, cam.position);
  float x = obz_vec3f_dot(d, right);
  float y = obz_vec3f_dot(d, up);
  float z = obz_vec3f_dot(d, fwd);

  // ---------------------- NEAR/FAR CLIPPING ----------------------
  /*
      The point must lie inside the view frustum depth range:

          z > near
          z < far

      If z is behind or beyond the camera’s view, reject it.
  */
  // ----------------------------------------------------------------
  if (z <= cam.near || z >= cam.far)
    return OBZ_ERR_CAMERA;

  // ---------------------- PERSPECTIVE PROJECTION ----------------------
  /*
      We use the simplified perspective division:

          f = 1 / tan(fov/2)

          ndc_x = (x * f) / (z * aspect)
          ndc_y = (y * f) / z

      This is equivalent to multiplying by the perspective projection matrix
      and then dividing by w.

      It maps camera-space to NDC (normalized device coordinates):
          ndc ∈ [-1, +1]
  */
  // ----------------------------------------------------------------------
  float f = 1.0f / tanf(cam.fov * 0.5f);

  // NDC coordinates (normalized device coords)
  float ndc_x = (x * f) / (z * cam.aspect);
  float ndc_y = (y * f) / z;

  // ---------------------- NDC -> SCREEN COORDINATES ----------------------
  /*
      Convert NDC [-1,1] to pixel space:

          px = (ndc_x * 0.5 + 0.5) * screen_width
          py = (1 - (ndc_y * 0.5 + 0.5)) * screen_height

      The Y-axis flips because screen coordinates grow downward.
  */
  // -----------------------------------------------------------------------
  *px = (int)(sw * 0.5f + ndc_x * sw * 0.5f);
  *py = (int)(sh * 0.5f - ndc_y * sh * 0.5f);

  return OBZ_OK;
}
