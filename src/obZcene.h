#ifndef OBZCENE_H
#define OBZCENE_H

#include "Obj/parser.h"
#include "SDL/Render/render.h"
#include "SDL/Scene/scene.h"
#include "obz_log.h"
#include <stdlib.h>

static const float W      = 1200;
static const float H      = 600;
static const float D      = 1000;
static const float margin = 20.0f;

inline static void print_camera_debug(const OBZ_Camera* cam)
{
  Vec3 fwd = obz_vec3_norm(cam->direction);

  printf("\r"
         "POS: %.2f %.2f %.2f | "
         "DIR: %.2f %.2f %.2f | "
         "YAW: %.2f | PITCH: %.2f   ",
         cam->position.x, cam->position.y, cam->position.z, fwd.x, fwd.y, fwd.z, cam->yaw,
         cam->pitch);

  fflush(stdout);
}

inline static void draw_crosshair(OBZ_RendererContext* rc, int cx, int cy, int length, int gap,
                                  OBZ_pixel color)
{
  // horizontal left
  for (int x = cx - length; x < cx - gap; x++)
    obz_render_pixel(rc, x, cy, 0, color);

  // horizontal right
  for (int x = cx + gap; x <= cx + length; x++)
    obz_render_pixel(rc, x, cy, 0, color);

  // vertical top
  for (int y = cy - length; y < cy - gap; y++)
    obz_render_pixel(rc, cx, y, 0, color);

  // vertical bottom
  for (int y = cy + gap; y <= cy + length; y++)
    obz_render_pixel(rc, cx, y, 0, color);
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
  }

  OBZ_Mesh3D* obj = obz_mesh_from_obj(&out);
  if (!obj || obj->noOfVerts == 0 || obj->noOfFaces == 0)
  {
    OBZ_LOG_ERROR(NULL, "init_scene: OBJ load failed or empty mesh");
    free(obj);
    exit(-1);
  }

  // ------------ Add mesh to scene ------------
  static Vec3 pos  = {0, 0, 0};
  static Rot3 rot  = {180, 0, 0};
  static Rot3 spin = {0, 0, 0};

  int mid = obz_scene_add_mesh(scene, obj, pos, rot, 1, spin);
  if (mid < 0)
  {
    OBZ_LOG_FATAL(NULL, "init_scene: failed to add OBJ mesh to scene");
    free(obj);
    exit(-1);
  }

  OBZ_LOG_TRACE(NULL, "Scene initialized with OBJ model and UV texture bound");
}

#endif /* OBZCENE_H */
