#include "SDL/Scene/scene.h"

/* ---------------- Internal API helpers --------------- */
OBZ_INTERNAL_API static void __OBZ_scene_ensure_capacity(OBZ_Scene* s, int need)
{
  if (s->capacity >= need)
    return;
  int cap = s->capacity ? s->capacity : 8;
  while (cap < need)
    cap *= 2;

  s->meshes    = obz_realloc(s->meshes, sizeof(OBZ_Mesh3D*) * cap);
  s->positions = obz_realloc(s->positions, sizeof(Vec3f) * cap);
  s->rotations = obz_realloc(s->rotations, sizeof(Rot3f) * cap);
  s->rotSpeeds = obz_realloc(s->rotSpeeds, sizeof(Rot3f) * cap);
  s->owned     = obz_realloc(s->owned, sizeof(OBZ_MeshOwnership) * cap);

  s->capacity = cap;
}

OBZ_INTERNAL_API static void __OBZ_scene_mesh_id_incr(OBZ_Scene* s) { s->id++; }

/* ---------------- Create / Destroy --------------- */
OBZ_Scene* obz_scene_create(void)
{
  OBZ_Scene* s = obz_calloc(1, sizeof(OBZ_Scene));
  return s;
}

void obz_scene_destroy(OBZ_Scene* s)
{
  if (!s)
    return;

  for (obz_count_t i = 0; i < s->id; i++)
  {
    if (s->owned[i] && s->meshes[i])
    {
      obz_mesh_free_all(s->meshes[i]);
      obz_free(s->meshes[i]);
    }
  }

  obz_free(s->meshes);
  obz_free(s->positions);
  obz_free(s->rotations);
  obz_free(s->rotSpeeds);
  obz_free(s->owned);

  obz_free(s);
}

OBZ_MeshID obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3f positions, Rot3f rotations,
                              Rot3f rotSpeeds, OBZ_MeshOwnership own_mesh)
{
  if (!mesh)
    return -1;

  OBZ_MeshID id = s->id;
  __OBZ_scene_ensure_capacity(s, id + 1);

  s->meshes[id]    = mesh;
  s->positions[id] = positions;
  s->rotations[id] = rotations;
  s->owned[id]     = own_mesh;
  s->rotSpeeds[id] = rotSpeeds;

  __OBZ_scene_mesh_id_incr(s);

  return id;
}

void obz_scene_update(OBZ_Scene* s, float dt)
{
  for (obz_count_t i = 0; i < s->id; i++)
  {
    Rot3f v = s->rotSpeeds[i];
    s->rotations[i].x += v.x * dt;
    s->rotations[i].y += v.y * dt;
    s->rotations[i].z += v.z * dt;
  }
  s->time += dt;
}

void obz_scene_iter_begin(OBZ_Scene* s, OBZ_SceneEntry* e) { e->index = -1; }

OBZ_Result obz_scene_iter_next(OBZ_Scene* s, OBZ_SceneEntry* e)
{
  if (++e->index >= s->id)
    return OBZ_ERR_SCENE;

  OBZ_MeshID i = e->index;
  e->mesh      = s->meshes[i];
  e->pos       = &s->positions[i];
  e->rot       = &s->rotations[i];
  e->rotSpeed  = &s->rotSpeeds[i];

  return OBZ_OK;
}
