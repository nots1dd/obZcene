#ifndef OBZ_SDL_SCENE_H
#define OBZ_SDL_SCENE_H

#include "SDL/Mesh/mesh.h"
#include "obz_macros.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct OBZ_Scene OBZ_Scene;

  typedef Vec3 Rot3;

  typedef struct
  {
    int         index;
    OBZ_Mesh3D* mesh;
    Vec3*       pos;
    Rot3*       rot;
    Rot3*       rotSpeed;
  } OBZ_SceneEntry;

  /* ---- Creation / Destruction ---- */
  OBZ_API OBZ_Scene* obz_scene_create(void);
  OBZ_API void       obz_scene_destroy(OBZ_Scene* s);

  /* ---- Add mesh ---- */
  OBZ_API int obz_scene_add_mesh(OBZ_Scene* s, OBZ_Mesh3D* mesh, Vec3 pos, Rot3 rot, int own_mesh,
                                 Rot3 rotSpeed);

  /* ---- Update ---- */
  OBZ_API void obz_scene_update(OBZ_Scene* s, float dt);

  /* ---- Iteration ---- */
  OBZ_API void obz_scene_iter_begin(OBZ_Scene* s, OBZ_SceneEntry* e);
  OBZ_API int  obz_scene_iter_next(OBZ_Scene* s, OBZ_SceneEntry* e);

  /* ---- Utility ---- */
  static inline Rot3 obz_deg_to_rad3(Rot3 d)
  {
    const float k = OBZ_ONE_RAD_IN_DEG;
    return (Rot3){d.x * k, d.y * k, d.z * k};
  }

#ifdef __cplusplus
}
#endif

#endif /* OBZ_SDL_SCENE_H */
