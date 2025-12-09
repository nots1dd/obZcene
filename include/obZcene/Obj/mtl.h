#ifndef OBZ_OBJ_MTL_PARSER_H
#define OBZ_OBJ_MTL_PARSER_H

#include "Obj/obj.h"
#include "obz_macros.h"

OBZ_BEGIN_CPP_DECLS

/* Find material index by name, returns -1 if not found */
OBZ_INTERNAL int __OBZ_find_material(const OBZ_ObjMesh* mesh, const char* name);

/* Push a new material and return its index (-1 on error) */
OBZ_INTERNAL int __OBZ_push_material(OBZ_ObjMesh* mesh, const OBZ_ObjMtl* mat);

OBZ_API OBZ_Result obz_mtl_load(const char* mtl_path, OBZ_ObjMesh* out);

OBZ_END_CPP_DECLS

#endif /* OBZ_OBJ_MTL_PARSER_H */
