#ifndef OBZCENE_H
#define OBZCENE_H

#include "SDL/Mesh/3dshapes.h"
#include "SDL/Scene/scene.h"
#include "obz_log.h"

static const float W      = 1000;
static const float H      = 400;
static const float D      = 1000;
static const float margin = 20.0f; // distance from wall

static OBZ_Mesh3D*  cube     = NULL;
static OBZ_Mesh3D*  sphere   = NULL;
static OBZ_Mesh3D*  pyramid  = NULL;
static OBZ_Mesh3D*  quad     = NULL;
static OBZ_Texture* quad_tex = NULL;

/* corner pyramids */
static const Vec3 pyramid_pos1 = {400, -150, -400};
static const Vec3 pyramid_pos2 = {400, -150, 400};
static const Vec3 pyramid_pos3 = {-400, -150, 400};
static const Vec3 pyramid_pos4 = {-400, -150, -400};

inline static void scene_add_room(OBZ_Scene* scene, float W, float H, float D, int tiles,
                                  Rot3 speed)
{
  float floorY = -H / 2;
  float ceilY  = H / 2;

  OBZ_Mesh3D* plane = NULL;

  // FLOOR
  plane   = malloc(sizeof(OBZ_Mesh3D));
  *plane  = make_wire_plane(W, D, tiles, tiles);
  int pid = obz_scene_add_mesh(scene, plane, (Vec3){0, floorY, 0}, (Rot3){-M_PI / 2, 0, 0},
                               COLOR_GRAY, 1, speed);
  obz_scene_set_texture(scene, pid, NULL, 7);

  // CEILING
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, D, tiles, tiles);
  pid = obz_scene_add_mesh(scene, plane, (Vec3){0, ceilY, 0}, (Rot3){M_PI / 2, 0, 0}, COLOR_GRAY, 1,
                           speed);
  obz_scene_set_texture(scene, pid, NULL, 8);

  // BACK WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, H, tiles, tiles);
  pid =
    obz_scene_add_mesh(scene, plane, (Vec3){0, 0, -D / 2}, (Rot3){0, 0, 0}, COLOR_GRAY, 1, speed);
  obz_scene_set_texture(scene, pid, NULL, 9);

  // FRONT WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, H, tiles, tiles);
  pid =
    obz_scene_add_mesh(scene, plane, (Vec3){0, 0, D / 2}, (Rot3){0, M_PI, 0}, COLOR_GRAY, 1, speed);
  obz_scene_set_texture(scene, pid, NULL, 10);
  // LEFT WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(D, H, tiles, tiles);
  pid = obz_scene_add_mesh(scene, plane, (Vec3){-W / 2, 0, 0}, (Rot3){0, -M_PI / 2, 0}, COLOR_GRAY,
                           1, speed);
  obz_scene_set_texture(scene, pid, NULL, 11);

  // RIGHT WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(D, H, tiles, tiles);
  pid = obz_scene_add_mesh(scene, plane, (Vec3){W / 2, 0, 0}, (Rot3){0, M_PI / 2, 0}, COLOR_GRAY, 1,
                           speed);
  obz_scene_set_texture(scene, pid, NULL, 12);
}

inline static void init_scene(OBZ_Scene* scene)
{
  cube     = malloc(sizeof(OBZ_Mesh3D));
  *cube    = make_cube(100);
  sphere   = malloc(sizeof(OBZ_Mesh3D));
  *sphere  = make_sphere(50, 12);
  pyramid  = malloc(sizeof(OBZ_Mesh3D));
  *pyramid = make_pyramid(80, 120);
  quad     = malloc(sizeof(OBZ_Mesh3D));
  *quad    = make_quad(30, 30);

  int cube_id = obz_scene_add_mesh(scene, cube, (Vec3){0, 0, 0}, (Rot3){0, 0, 0}, COLOR_RED, 1,
                                   (Rot3){0, 0, 0});
  obz_scene_set_texture(scene, cube_id, NULL, 0); // ensure texture slot struct exists

  scene_add_room(scene, W, H, D, 4, (Rot3){0, 0, 0});

  int pid1 = obz_scene_add_mesh(scene, pyramid, pyramid_pos1, (Rot3){0, 0, 0}, COLOR_BLUE, 1,
                                (Rot3){15, 30, 45});
  obz_scene_set_texture(scene, pid1, NULL, 1); // ensure texture slot struct exists
  int pid2 = obz_scene_add_mesh(scene, pyramid, pyramid_pos2, (Rot3){0, 0, 0}, COLOR_MAGENTA, 1,
                                (Rot3){15, 30, 45});
  obz_scene_set_texture(scene, pid2, NULL, 2); // ensure texture slot struct exists
  int pid3 = obz_scene_add_mesh(scene, pyramid, pyramid_pos3, (Rot3){0, 0, 0}, COLOR_LIME, 1,
                                (Rot3){15, 30, 45});
  obz_scene_set_texture(scene, pid3, NULL, 3); // ensure texture slot struct exists
  int pid4 = obz_scene_add_mesh(scene, pyramid, pyramid_pos4, (Rot3){0, 0, 0}, COLOR_WHITE, 1,
                                (Rot3){15, 30, 45});
  obz_scene_set_texture(scene, pid4, NULL, 4); // ensure texture slot struct exists

  OBZ_LOG_TRACE(NULL, "Added pyramids with IDs %d, %d, %d, %d", pid1, pid2, pid3, pid4);

  /* sphere */
  int sid = obz_scene_add_mesh(scene, sphere, (Vec3){200, 60, 0}, (Rot3){0, 0, 0}, COLOR_CYAN, 1,
                               (Rot3){0, 60, 0});
  obz_scene_set_texture(scene, sid, NULL, 5);

  OBZ_LOG_TRACE(NULL, "Added sphere with ID %d", sid);

  /* quad */
  int qid = obz_scene_add_mesh(scene, quad, (Vec3){100, 100, 0}, (Rot3){0, 0, 0}, COLOR_WHITE, 1,
                               (Rot3){0, 0, 0});

  // Load texture for quad
  quad_tex = obz_tex_load_png("assets/obzen.png");
  if (quad_tex)
  {
    // we are setting to 0 coz this is our FIRST non null texture so we can override it without
    // a problem.
    obz_scene_set_texture(scene, qid, quad_tex, 0);
    OBZ_LOG_TRACE(NULL, "Added quad with texture with ID %d", qid);
  }
  else
  {
    OBZ_LOG_WARN(NULL, "Could not load PNG onto the texture!");
  }

  OBZ_LOG_TRACE(NULL, "Scene initialized.");
}

#endif
