#ifndef OBZ_SCENE_H
#define OBZ_SCENE_H

#include "SDL/Colors/colors.h"
#include "SDL/Mesh/mesh.h"
#include "SDL/Textures/textures.h"
#include "obz_macros.h"
#include <stdint.h>

typedef struct OBZ_Scene OBZ_Scene;

typedef Vec3 Rot3;

typedef struct
{
  int         index;
  OBZ_Mesh3D* mesh;
  Vec3*       pos;
  Rot3*       rot;
  Rot3*       rotSpeed;
  OBZ_Color*  color;
} OBZ_SceneEntry;

/* ---- Creation / Destruction ---- */
OBZ_API OBZ_Scene* obz_scene_create(void);
OBZ_API void       obz_scene_destroy(OBZ_Scene* s);

/* ---- Add mesh ---- */
OBZ_API int obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3 pos, Rot3 rot, OBZ_Color color,
                               int own_mesh, Rot3 rotSpeed);

/* ---- Update ---- */
OBZ_API void obz_scene_update(OBZ_Scene* s, float dt);

/* ---- Textures ---- */
OBZ_API void obz_scene_set_texture(OBZ_Scene* s, int mesh_id, OBZ_Texture* tex, int slot);
OBZ_API OBZ_MeshTextures* obz_scene_get_mesh_textures(OBZ_Scene* s, int mesh_id);

/* ---- Iteration ---- */
OBZ_API void obz_scene_iter_begin(OBZ_Scene* s, OBZ_SceneEntry* e);
OBZ_API int  obz_scene_iter_next(OBZ_Scene* s, OBZ_SceneEntry* e);

/* ---- Utility ---- */
static inline Rot3 obz_deg_to_rad3(Rot3 d)
{
  const float k = OBZ_ONE_RAD_IN_DEG;
  return (Rot3){d.x * k, d.y * k, d.z * k};
}

#endif
