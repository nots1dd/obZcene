#include "SDL/Scene/scene.h"
#include "obz_log.h"
#include <stdlib.h>

struct OBZ_Scene
{
  OBZ_Mesh3D**       meshes;
  Vec3*              positions;
  Rot3*              rotations;
  Rot3*              rotSpeeds;
  OBZ_Color*         colors;
  uint8_t*           owned;
  OBZ_MeshTextures** textures;

  int   count;
  int   capacity;
  float time;
};

/* ---------------- Internal Helper --------------- */
static void __OBZ_scene_ensure_capacity(OBZ_Scene* s, int need)
{
  if (s->capacity >= need)
    return;
  int oldcap = s->capacity;
  int cap    = s->capacity ? s->capacity : 8;
  while (cap < need)
    cap *= 2;

  s->meshes    = realloc(s->meshes, sizeof(OBZ_Mesh3D*) * cap);
  s->positions = realloc(s->positions, sizeof(Vec3) * cap);
  s->rotations = realloc(s->rotations, sizeof(Rot3) * cap);
  s->rotSpeeds = realloc(s->rotSpeeds, sizeof(Rot3) * cap);
  s->colors    = realloc(s->colors, sizeof(OBZ_Color) * cap);
  s->owned     = realloc(s->owned, sizeof(uint8_t) * cap);
  s->textures  = realloc(s->textures, sizeof(OBZ_MeshTextures*) * cap);

  for (int i = oldcap; i < cap; i++)
    s->textures[i] = NULL;

  s->capacity = cap;
}

/* ---------------- Create / Destroy --------------- */
OBZ_Scene* obz_scene_create(void)
{
  OBZ_Scene* s = calloc(1, sizeof(OBZ_Scene));
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
      free_mesh(s->meshes[i]);
      free(s->meshes[i]);
    }

    if (s->textures && s->textures[i])
    {
      for (int t = 0; t < OBZ_MAX_MESH_TEXTURES; t++)
        if (s->textures[i]->textures[t])
          obz_tex_free(s->textures[i]->textures[t]);

      free(s->textures[i]);
    }
  }

  free(s->meshes);
  free(s->positions);
  free(s->rotations);
  free(s->rotSpeeds);
  free(s->colors);
  free(s->owned);
  free(s->textures);

  free(s);
}

/* ---------------- Add Mesh --------------- */
int obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3 pos, Rot3 rot, OBZ_Color color,
                       int own_mesh, Rot3 speed)
{
  if (!mesh)
    return -1;

  int id = s->count;
  __OBZ_scene_ensure_capacity(s, id + 1);

  s->meshes[id]    = mesh;
  s->positions[id] = pos;
  s->rotations[id] = rot;
  s->colors[id]    = color;
  s->owned[id]     = own_mesh ? 1 : 0;
  s->rotSpeeds[id] = speed;
  s->textures[id]  = NULL;

  s->count++;
  return id;
}

/* ---------------- Update --------------- */
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

/* ---------------- Textures --------------- */
void obz_scene_set_texture(OBZ_Scene* s, int mesh_id, OBZ_Texture* tex, int slot)
{
  if (!s)
    return;
  if (mesh_id < 0 || mesh_id >= s->count)
    return;
  if (slot < 0)
    slot = 0;
  if (slot >= OBZ_MAX_MESH_TEXTURES)
  {
    OBZ_LOG_WARN(NULL, "Texture slot %d out of range, clamping to %d", slot,
                 OBZ_MAX_MESH_TEXTURES - 1);
    slot = OBZ_MAX_MESH_TEXTURES - 1;
  }

  if (!s->textures[mesh_id])
    s->textures[mesh_id] = obz_tex_create_for_mesh(s->meshes[mesh_id]);

  obz_tex_bind_to_mesh(s->textures[mesh_id], tex, slot);

  OBZ_LOG_DEBUG(NULL, "Bound tex %p to mesh %d slot %d (mesh_textures %p)", (void*)tex, mesh_id,
                slot, (void*)s->textures[mesh_id]);
}

OBZ_MeshTextures* obz_scene_get_mesh_textures(OBZ_Scene* s, int mesh_id)
{
  if (!s)
    return NULL;
  if (mesh_id < 0 || mesh_id >= s->count)
    return NULL;

  return s->textures[mesh_id];
}

/* ---------------- Iterator --------------- */
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
  e->color    = &s->colors[i];

  return 1;
}
