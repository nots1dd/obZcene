#ifndef OBZ_SDL_SCENE_H
#define OBZ_SDL_SCENE_H

#include "SDL/Colors/colors.h"
#include "SDL/Mesh/mesh.h"
#include "SDL/Textures/textures.h"
#include "macros.h"
#include <stdlib.h>

typedef Vec3 Rot3;

typedef struct
{
  OBZ_Mesh3D**       meshes; /* pointers to meshes (do not copy Mesh3D by value) */
  Vec3*              positions;
  Rot3*              rotations;
  OBZ_Color*         colors;
  OBZ_MeshTextures** textures;
  ui8*               owned; /* 1 => this scene is owner and must free the mesh */
  int                count;
  int                capacity;
  Rot3*              rotSpeeds;

  float time;
} OBZ_Scene;

static OBZ_Scene g_scene = {0};

inline static void scene_init_textures_for_mesh(int mesh_id)
{
  if (mesh_id < 0 || mesh_id >= g_scene.count)
    return;

  if (!g_scene.textures)
    g_scene.textures = calloc(g_scene.capacity, sizeof(OBZ_MeshTextures*));

  if (!g_scene.textures[mesh_id])
  {
    g_scene.textures[mesh_id] = obz_tex_create_for_mesh(g_scene.meshes[mesh_id]);
  }
}

inline static void scene_bind_texture_to_mesh(int mesh_id, OBZ_Texture* tex, int slot)
{
  if (mesh_id < 0 || mesh_id >= g_scene.count || !g_scene.textures)
    return;

  if (!g_scene.textures[mesh_id])
    scene_init_textures_for_mesh(mesh_id);

  obz_tex_bind_to_mesh(g_scene.textures[mesh_id], tex, slot);
}

inline static void scene_free_textures(void)
{
  if (!g_scene.textures)
    return;

  for (int i = 0; i < g_scene.count; i++)
  {
    if (g_scene.textures[i])
    {
      for (int s = 0; s < OBZ_MAX_MESH_TEXTURES; s++)
      {
        if (g_scene.textures[i]->textures[s])
          obz_tex_free(g_scene.textures[i]->textures[s]);
      }
      free(g_scene.textures[i]);
    }
  }

  free(g_scene.textures);
  g_scene.textures = NULL;
}

inline static void scene_update(float dt)
{
  for (int i = 0; i < g_scene.count; i++)
  {
    Vec3 s = g_scene.rotSpeeds[i];
    if (s.x != 0 || s.y != 0 || s.z != 0)
    {
      g_scene.rotations[i].x += s.x * dt;
      g_scene.rotations[i].y += s.y * dt;
      g_scene.rotations[i].z += s.z * dt;
    }
  }

  g_scene.time += dt;
}

inline static Vec3 deg_to_rad3(Rot3 d)
{
  const float k = OBZ_ONE_RAD_IN_DEG;
  return (Rot3){d.x * k, d.y * k, d.z * k};
}

/* ensure capacity (doubling) */
inline static void scene_ensure_capacity(int need)
{
  if (g_scene.capacity >= need)
    return;
  int cap = g_scene.capacity ? g_scene.capacity : 8;
  while (cap < need)
    cap *= 2;
  g_scene.meshes    = realloc(g_scene.meshes, sizeof(OBZ_Mesh3D*) * cap);
  g_scene.positions = realloc(g_scene.positions, sizeof(Vec3) * cap);
  g_scene.rotations = realloc(g_scene.rotations, sizeof(Rot3) * cap);
  g_scene.rotSpeeds = realloc(g_scene.rotSpeeds, sizeof(Rot3) * cap);
  g_scene.colors    = realloc(g_scene.colors, sizeof(OBZ_Color) * cap);
  g_scene.owned     = realloc(g_scene.owned, sizeof(ui8) * cap);
  g_scene.textures  = realloc(g_scene.textures, sizeof(OBZ_MeshTextures*) * cap);
  g_scene.capacity  = cap;
}

/* Add a mesh pointer to the scene.
   If own == 1, scene_free_all() will call free_mesh() and free() the pointer.
   If own == 0, scene only references the mesh (do not free it in scene_free_all). */
inline static int scene_add_mesh_ptr(OBZ_Mesh3D* m, Vec3 pos, Rot3 rot, OBZ_Color col, int own,
                                     Rot3 rotSpeed)
{
  if (!m)
    return -1;
  int id = g_scene.count;
  scene_ensure_capacity(g_scene.count + 1);

  g_scene.meshes[id]    = m;
  g_scene.positions[id] = pos;
  g_scene.rotations[id] = rot;
  g_scene.colors[id]    = col;
  g_scene.owned[id]     = (ui8)(own ? 1 : 0);
  g_scene.rotSpeeds[id] = rotSpeed;

  if (g_scene.textures)
  {
    g_scene.textures[id] = obz_tex_create_for_mesh(m); // allocate on heap
    if (!g_scene.textures[id])
    {
      printf("[WARN] Failed to create textures for mesh %d\n", id);
      g_scene.textures[id] = NULL;
    }
  }

  g_scene.count++;
  return id;
}

/* Free only meshes that scene owns, and free arrays */
inline static void scene_free_all(void)
{
  for (int i = 0; i < g_scene.count; i++)
  {
    if (g_scene.owned && g_scene.owned[i])
    {
      OBZ_Mesh3D* m = g_scene.meshes[i];
      if (m)
      {
        free_mesh(m); /* frees verts/faces inside */
        free(m);      /* free the container struct */
      }
    }
  }

  free(g_scene.meshes);
  free(g_scene.positions);
  free(g_scene.rotations);
  free(g_scene.colors);
  free(g_scene.owned);
  free(g_scene.rotSpeeds);
  scene_free_textures();

  g_scene.meshes    = NULL;
  g_scene.positions = NULL;
  g_scene.rotSpeeds = NULL;
  g_scene.rotations = NULL;
  g_scene.colors    = NULL;
  g_scene.owned     = NULL;
  g_scene.count     = 0;
  g_scene.capacity  = 0;
  g_scene.time      = 0.0f;
}

inline static int scene_add_pyramid(OBZ_Mesh3D* pyramid_ptr, Vec3 pos, Rot3 rot_deg, Rot3 speed_deg,
                                    OBZ_Color col)
{
  Rot3 rotRad   = deg_to_rad3(rot_deg);
  Vec3 speedRad = deg_to_rad3(speed_deg);

  return scene_add_mesh_ptr(pyramid_ptr, pos, rotRad, col, 0, speedRad);
}

inline static int scene_add_sphere(OBZ_Mesh3D* sphere_ptr, float orbit_radius, float baseY,
                                   Rot3 rot_deg, Rot3 speed_deg, OBZ_Color col)
{
  Vec3 rotRad   = deg_to_rad3(rot_deg);
  Vec3 speedRad = deg_to_rad3(speed_deg);

  return scene_add_mesh_ptr(sphere_ptr, (Vec3){orbit_radius, baseY, 0}, rotRad, col, 0, speedRad);
}

inline static void scene_add_room(float W, float H, float D, int tiles, Rot3 base_rot, Rot3 speed)
{
  float floorY = -H / 2;
  float ceilY  = H / 2;

  OBZ_Mesh3D* plane = NULL;

  // FLOOR
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, D, tiles, tiles);
  int pid =
    scene_add_mesh_ptr(plane, (Vec3){0, floorY, 0}, (Rot3){-M_PI / 2, 0, 0}, COLOR_GRAY, 1, speed);
  scene_init_textures_for_mesh(pid);

  // CEILING
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, D, tiles, tiles);
  pid =
    scene_add_mesh_ptr(plane, (Vec3){0, ceilY, 0}, (Rot3){M_PI / 2, 0, 0}, COLOR_GRAY, 1, speed);
  scene_init_textures_for_mesh(pid);

  // BACK WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, H, tiles, tiles);
  pid    = scene_add_mesh_ptr(plane, (Vec3){0, 0, -D / 2}, (Rot3){0, 0, 0}, COLOR_GRAY, 1, speed);
  scene_init_textures_for_mesh(pid);

  // FRONT WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, H, tiles, tiles);
  pid    = scene_add_mesh_ptr(plane, (Vec3){0, 0, D / 2}, (Rot3){0, M_PI, 0}, COLOR_GRAY, 1, speed);
  scene_init_textures_for_mesh(pid);
  // LEFT WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(D, H, tiles, tiles);
  pid =
    scene_add_mesh_ptr(plane, (Vec3){-W / 2, 0, 0}, (Rot3){0, -M_PI / 2, 0}, COLOR_GRAY, 1, speed);
  scene_init_textures_for_mesh(pid);

  // RIGHT WALL
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(D, H, tiles, tiles);
  pid =
    scene_add_mesh_ptr(plane, (Vec3){W / 2, 0, 0}, (Rot3){0, M_PI / 2, 0}, COLOR_GRAY, 1, speed);
  scene_init_textures_for_mesh(pid);
}

#endif
