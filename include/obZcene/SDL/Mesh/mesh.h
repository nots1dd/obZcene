#ifndef OBZ_SDL_MESH_H
#define OBZ_SDL_MESH_H

#include "Math/vec.h"
#include "Obj/obj.h"

OBZ_BEGIN_CPP_DECLS

typedef struct
{
  Vec3f* verts; // vertices
  Vec2f* uvs;   // texcoords
  Vec3f* norms; // normals

  int* indices;      // vertex indices (triangles)
  int* uv_indices;   // uv indices
  int* norm_indices; // normal indices

  OBZ_ObjMtl* materials;      // All materials loaded from .mtl
  int         material_count; // # of materials

  int* face_mtl_id; // length = noOfFaces
                    // each triangle -> material index

  /* ------------ COUNTS ------------ */
  obz_count_t noOfVerts;
  obz_count_t noOfUVs;
  obz_count_t noOfNorms;
  obz_count_t noOfFaces;
} OBZ_Mesh3D;

OBZ_API void obz_mesh_free_all(OBZ_Mesh3D* mesh);

OBZ_API OBZ_Mesh3D* obz_mesh_from_obj(const OBZ_ObjMesh* src);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_MESH_H */
