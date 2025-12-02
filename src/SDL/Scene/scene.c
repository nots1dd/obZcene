#include "SDL/Scene/scene.h"

struct OBZ_Scene
{
  OBZ_Mesh3D** meshes;
  Vec3*        positions;
  Rot3*        rotations;
  Rot3*        rotSpeeds;
  uint8_t*     owned;

  int   count;
  int   capacity;
  float time;
};

/* ---------------- Internal Helper --------------- */
static void __OBZ_scene_ensure_capacity(OBZ_Scene* s, int need)
{
  if (s->capacity >= need)
    return;
  int cap = s->capacity ? s->capacity : 8;
  while (cap < need)
    cap *= 2;

  s->meshes    = obz_realloc(s->meshes, sizeof(OBZ_Mesh3D*) * cap);
  s->positions = obz_realloc(s->positions, sizeof(Vec3) * cap);
  s->rotations = obz_realloc(s->rotations, sizeof(Rot3) * cap);
  s->rotSpeeds = obz_realloc(s->rotSpeeds, sizeof(Rot3) * cap);
  s->owned     = obz_realloc(s->owned, sizeof(uint8_t) * cap);

  s->capacity = cap;
}

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

  for (int i = 0; i < s->count; i++)
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

int obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3 pos, Rot3 rot, int own_mesh, Rot3 speed)
{
  if (!mesh)
    return -1;

  int id = s->count;
  __OBZ_scene_ensure_capacity(s, id + 1);

  s->meshes[id]    = mesh;
  s->positions[id] = pos;
  s->rotations[id] = rot;
  s->owned[id]     = own_mesh ? 1 : 0;
  s->rotSpeeds[id] = speed;

  s->count++;
  return id;
}

void obz_scene_update(OBZ_Scene* s, float dt)
{
  for (int i = 0; i < s->count; i++)
  {
    Rot3 v = s->rotSpeeds[i];
    s->rotations[i].x += v.x * dt;
    s->rotations[i].y += v.y * dt;
    s->rotations[i].z += v.z * dt;
  }
  s->time += dt;
}

void obz_scene_iter_begin(OBZ_Scene* s, OBZ_SceneEntry* e) { e->index = -1; }

int obz_scene_iter_next(OBZ_Scene* s, OBZ_SceneEntry* e)
{
  if (++e->index >= s->count)
    return 0;

  int i       = e->index;
  e->mesh     = s->meshes[i];
  e->pos      = &s->positions[i];
  e->rot      = &s->rotations[i];
  e->rotSpeed = &s->rotSpeeds[i];

  return 1;
}
