#ifndef OBZ_OBZ_H
#define OBZ_OBZ_H

#include "Math/vec.h"
#include "SDL/Textures/textures.h"
#include "Utils/dynarray.h"
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
  obz_arr(Vec3) positions;     /* Vec3 */
  obz_arr(Vec2) texcoords;     /* Vec2 */
  obz_arr(Vec3) normals;       /* Vec3 */
  obz_arr(OBZ_ObjIndex) faces; /* OBZ_ObjIndex (triangulated) */

  obz_arr(int) face_mtl_id;      /* int per triangle (index into materials) */
  obz_arr(OBZ_ObjMtl) materials; /* OBZ_Material[] */
} OBZ_ObjMesh;

/* Material representation parsed from .mtl */
typedef struct
{
  char         name[64];
  Vec3f        Kd;     /* diffuse color (0..1) */
  Vec3f        Ka;     /* ambient color (0..1) */
  Vec3f        Ks;     /* specular color (0..1) */
  float        Ns;     /* shininess */
  OBZ_Texture* map_Kd; /* optional texture path (map_Kd) */
} OBZ_ObjMtl;

OBZ_END_CPP_DECLS

#endif /* OBZ_OBJ_H */
