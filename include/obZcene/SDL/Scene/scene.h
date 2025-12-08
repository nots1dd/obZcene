#ifndef OBZ_SDL_SCENE_H
#define OBZ_SDL_SCENE_H

#include "SDL/Mesh/mesh.h"
#include "obz_macros.h"
#include <stdint.h>

OBZ_BEGIN_CPP_DECLS

typedef Vec3d Rot3d;
typedef Vec3f Rot3f;

typedef obz_count_t OBZ_MeshID;

static const OBZ_MeshID MESH_NOT_FOUND = -1;

typedef struct
{
  OBZ_MeshID  index; // idx of all the mesh ids present in the scene
  OBZ_Mesh3D* mesh;
  Vec3f*      pos;
  Rot3f*      rot;
  Rot3f*      rotSpeed;
} OBZ_SceneEntry;

typedef enum : uint8_t
{
  OBZ_SCENE_DONT_OWN_MESH = 0,
  OBZ_SCENE_OWN_MESH      = 1,
} OBZ_MeshOwnership;

typedef struct
{
  OBZ_Mesh3D**       meshes;
  Vec3f*             positions;
  Rot3f*             rotations;
  Rot3f*             rotSpeeds;
  OBZ_MeshOwnership* owned;

  OBZ_MeshID id;
  int        capacity;
  float      time;
} OBZ_Scene;

/* ---- Creation / Destruction ---- */
OBZ_API OBZ_Scene* obz_scene_create(void);
OBZ_API void       obz_scene_destroy(OBZ_Scene* s);

/* ---- Add mesh ---- */
OBZ_API OBZ_MeshID obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3f positions,
                                      Rot3f rotations, Rot3f rotSpeeds, OBZ_MeshOwnership own_mesh);

/* ---- Update ---- */
OBZ_API void obz_scene_update(OBZ_Scene* s, float dt);

/* ---- Iteration ---- */
OBZ_API void       obz_scene_iter_begin(OBZ_Scene* s, OBZ_SceneEntry* e);
OBZ_API OBZ_Result obz_scene_iter_next(OBZ_Scene* s, OBZ_SceneEntry* e);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_SCENE_H */
