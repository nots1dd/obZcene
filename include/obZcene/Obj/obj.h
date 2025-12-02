#ifndef OBZ_OBZ_H
#define OBZ_OBZ_H

#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
#include "Utils/vec.h"

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
  Vec3         Kd;     /* diffuse color (0..1) */
  Vec3         Ka;     /* ambient color (0..1) */
  Vec3         Ks;     /* specular color (0..1) */
  float        Ns;     /* shininess */
  OBZ_Texture* map_Kd; /* optional texture path (map_Kd) */
} OBZ_Material;

#endif
