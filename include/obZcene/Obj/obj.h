#ifndef OBZ_OBZ_H
#define OBZ_OBZ_H

#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
#include "Math/vec.h"
#include "obz_macros.h"

OBZ_BEGIN_CPP_DECLS

#ifndef OBZ_OBJ_LINE_MAX
#define OBZ_OBJ_LINE_MAX 1024
#endif

/* Face index set: OBJ faces can have v/vt/vn (1-based in file) */
typedef struct
{
  int v, vt, vn; /* as read from file (1-based). 0 means missing. */
} OBZ_ObjIndex;

/* Parsed OBJ mesh with material info */
typedef struct
{
  OBZ_DynArray positions; /* Vec3 */
  OBZ_DynArray texcoords; /* Vec2 */
  OBZ_DynArray normals;   /* Vec3 */
  OBZ_DynArray faces;     /* OBZ_ObjIndex (triangulated) */

  OBZ_DynArray face_mtl_id; /* int per triangle (index into materials) */
  OBZ_DynArray materials;   /* OBZ_Material[] */
} OBZ_ObjMesh;

/* Material representation parsed from .mtl */
typedef struct
{
  char         name[64];
  Vec3d        Kd;     /* diffuse color (0..1) */
  Vec3d        Ka;     /* ambient color (0..1) */
  Vec3d        Ks;     /* specular color (0..1) */
  double       Ns;     /* shininess */
  OBZ_Texture* map_Kd; /* optional texture path (map_Kd) */
} OBZ_ObjMtl;

OBZ_END_CPP_DECLS

#endif /* OBZ_OBJ_H */
