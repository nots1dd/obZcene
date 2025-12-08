#ifndef OBZ_SDL_SCENE_H
#define OBZ_SDL_SCENE_H

#include "SDL/Mesh/mesh.h"
#include "obz_macros.h"
#include <stdint.h>

OBZ_BEGIN_CPP_DECLS

typedef Vec3d Rot3d;
typedef Vec3f Rot3f;

typedef struct
{
  int         index;
  OBZ_Mesh3D* mesh;
  Vec3d*      pos;
  Rot3d*      rot;
  Rot3d*      rotSpeed;
} OBZ_SceneEntry;

typedef enum : uint8_t
{
  OBZ_SCENE_DONT_OWN_MESH = 0,
  OBZ_SCENE_OWN_MESH      = 1,
} OBZ_MeshOwnership;

typedef struct
{
  OBZ_Mesh3D**       meshes;
  Vec3d*             positions;
  Rot3d*             rotations;
  Rot3d*             rotSpeeds;
  OBZ_MeshOwnership* owned;

  int   count;
  int   capacity;
  float time;
} OBZ_Scene;

/* ---- Creation / Destruction ---- */
OBZ_API OBZ_Scene* obz_scene_create(void);
OBZ_API void       obz_scene_destroy(OBZ_Scene* s);

/* ---- Add mesh ---- */
OBZ_API int obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3d pos, Rot3d rot, int own_mesh,
                               Rot3d rotSpeed);

/* ---- Update ---- */
OBZ_API void obz_scene_update(OBZ_Scene* s, double dt);

/* ---- Iteration ---- */
OBZ_API void obz_scene_iter_begin(OBZ_Scene* s, OBZ_SceneEntry* e);
OBZ_API int  obz_scene_iter_next(OBZ_Scene* s, OBZ_SceneEntry* e);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_SCENE_H */
