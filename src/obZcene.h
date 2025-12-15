#ifndef OBZCENE_H
#define OBZCENE_H

#include "Obj/parser.h"
#include "SDL/Render/render.h"
#include "SDL/Scene/scene.h"
#include "SDL/keymaps.h"
#include "obz_log.h"
#include <stdlib.h>

static const float g_W      = 1200;
static const float g_H      = 600;
static const float g_D      = 1000;
static const float g_margin = 20.0f;

static const OBZ_CamBound g_x_axis = {-g_W / 2 + g_margin, g_W / 2 - g_margin};
static const OBZ_CamBound g_y_axis = {-g_H / 2 + g_margin, g_H / 2 - g_margin};
static const OBZ_CamBound g_z_axis = {-g_D / 2 + g_margin, g_D / 2 - g_margin};

// static void print_camera_debug(const OBZ_Camera* cam)
// {
//   Vec3f fwd = obz_vec3f_norm(cam->direction);
//
//   printf("\r"
//          "POS: %.2f %.2f %.2f | "
//          "DIR: %.2f %.2f %.2f | "
//          "YAW: %.2f | PITCH: %.2f   ",
//          cam->position.x, cam->position.y, cam->position.z, fwd.x, fwd.y, fwd.z, cam->rot.yaw,
//          cam->rot.pitch);
//
//   fflush(stdout);
// }

inline static void draw_crosshair(OBZ_RendererContext* rc, int cx, int cy, int length, int gap,
                                  OBZ_Color color)
{

  const OBZ_pixel pixel_color = __OBZ_pack_color(color);

  // horizontal left
  for (int x = cx - length; x < cx - gap; x++)
    obz_render_pixel(rc, x, cy, 0, pixel_color);

  // horizontal right
  for (int x = cx + gap; x <= cx + length; x++)
    obz_render_pixel(rc, x, cy, 0, pixel_color);

  // vertical top
  for (int y = cy - length; y < cy - gap; y++)
    obz_render_pixel(rc, cx, y, 0, pixel_color);

  // vertical bottom
  for (int y = cy + gap; y <= cy + length; y++)
    obz_render_pixel(rc, cx, y, 0, pixel_color);
}

void camera_move(OBZ_Camera* cam, const ui8* keyboard, float speed)
{
  Vec3f input = {0, 0, 0};

  if (keyboard[KC_W])
    input.z += 1;
  if (keyboard[KC_S])
    input.z -= 1;
  if (keyboard[KC_D])
    input.x += 1;
  if (keyboard[KC_A])
    input.x -= 1;
  if (keyboard[KC_SPACE])
    input.y += 1;
  if (keyboard[KC_BACKSPACE])
    input.y -= 1;

  if (obz_vec3f_len(input) < 1e-6f)
  {
    cam->velocity = obz_vec3f(0, 0, 0);
    return;
  }

  input = obz_vec3f_norm(input);

  Vec3f fwd = obz_vec3f_norm(cam->direction);

  Vec3f right = obz_vec3f_cross(g_camera_world_up, fwd);
  right       = obz_vec3f_norm(right);

  if (obz_vec3f_len(right) < 1e-6f)
    right = obz_vec3f_norm(obz_vec3f_cross((Vec3f){0, 0, 1}, fwd));

  Vec3f up = obz_vec3f_cross(fwd, right);

  // Compute velocity
  cam->velocity = obz_vec3_add(
    obz_vec3_add(obz_vec3f_mulf(fwd, input.z * speed), obz_vec3f_mulf(right, input.x * speed)),
    obz_vec3f_mulf(up, input.y * speed));
}

inline static void init_scene(OBZ_Scene* scene, const char* obj_file_path)
{
  if (!scene)
  {
    OBZ_LOG_WARN(NULL, "init_scene: scene is NULL");
    return;
  }

  OBZ_ObjMesh out;
  if (obz_obj_load(obj_file_path, &out) != OBZ_OK)
  {
    OBZ_LOG_FATAL(NULL, "init_scene: failed to load OBJ model from asset!!");
    exit(-1);
    return;
  }

  OBZ_Mesh3D* obj = obz_mesh_from_obj(&out);
  if (!obj || obj->noOfVerts == 0 || obj->noOfFaces == 0)
  {
    OBZ_LOG_ERROR(NULL, "init_scene: OBJ load failed or empty mesh");
    free(obj);
    exit(-1);
    return;
  }

  // ------------ Add mesh to scene ------------
  static Vec3f pos  = {0, 0, 0};
  static Rot3f rot  = {0, 0, 0};
  static Rot3f spin = {0, 0, 0};

  int mid = obz_scene_add_mesh(scene, obj, pos, rot, spin, OBZ_SCENE_OWN_MESH);
  if (mid < 0)
  {
    OBZ_LOG_FATAL(NULL, "init_scene: failed to add OBJ mesh to scene");
    free(obj);
    exit(-1);
    return;
  }

  OBZ_LOG_TRACE(NULL, "Scene initialized with OBJ model and UV texture bound");
}

#endif /* OBZCENE_H */
