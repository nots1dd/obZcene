#ifndef OBZ_OBJ_PARSER_H
#define OBZ_OBJ_PARSER_H

#include "Obj/obj.h"
#include "obz_macros.h"
#include "obz_types.h"

OBZ_BEGIN_CPP_DECLS

OBZ_API void obz_objmesh_init(OBZ_ObjMesh* objmesh);
OBZ_API void obz_objmesh_free(OBZ_ObjMesh* objmesh);

/* parse v/vt/vn triplet (keeps values as read from file, 1-based; 0 if missing) */
OBZ_API OBZ_ObjIndex __OBZ_parse_index(const char** s);

/* triangulate face stored in tmp[] (count entries). Push indices into faces array.
   Returns the number of triangles produced (count - 2). */
OBZ_API int __OBZ_triangulate_and_push(OBZ_DynArray* faces, OBZ_ObjIndex* tmp, int count);

/* ------------------------------
   Main OBJ loader (with implicit mtl loading)
   - If the OBJ contains `mtllib <file>`, the MTL will be loaded automatically
   - usemtl switches current material; faces after that use that material
   ------------------------------ */
OBZ_API OBZ_Result obz_obj_load(const char* path, OBZ_ObjMesh* out);

OBZ_END_CPP_DECLS

#endif /* OBZ_OBJ_PARSER_H */
