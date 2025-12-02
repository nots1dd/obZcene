#ifndef OBZ_SDL_MESH_H
#define OBZ_SDL_MESH_H

#include "Obj/parser.h"
#include "Utils/vec.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct
  {
    Vec3* verts; // vertices
    Vec2* uvs;   // texcoords
    Vec3* norms; // normals

    int* indices;      // vertex indices (triangles)
    int* uv_indices;   // uv indices
    int* norm_indices; // normal indices

    OBZ_Material* materials;      // All materials loaded from .mtl
    int           material_count; // # of materials

    int* face_mtl_id; // length = noOfFaces
                      // each triangle -> material index

    /* ------------ COUNTS ------------ */
    obz_count_t noOfVerts;
    obz_count_t noOfUVs;
    obz_count_t noOfNorms;
    obz_count_t noOfFaces;
  } OBZ_Mesh3D;

  inline static void obz_mesh_free_all(OBZ_Mesh3D* mesh)
  {
    if (!mesh)
      return;

    obz_free(mesh->verts);
    obz_free(mesh->uvs);
    obz_free(mesh->norms);

    obz_free(mesh->indices);
    obz_free(mesh->uv_indices);
    obz_free(mesh->norm_indices);

    obz_free(mesh->face_mtl_id);
    obz_free(mesh->materials);

    mesh->noOfVerts      = 0;
    mesh->noOfUVs        = 0;
    mesh->noOfNorms      = 0;
    mesh->noOfFaces      = 0;
    mesh->material_count = 0;
  }

  inline static OBZ_Mesh3D* obz_mesh_from_obj(const OBZ_ObjMesh* src)
  {
    if (!src)
      return NULL;

    int num_idx_entries = (int)src->faces.size;
    if (num_idx_entries <= 0)
      return NULL;

    obz_count_t   num_tris = num_idx_entries / 3;
    OBZ_ObjIndex* fbase    = obz_arr_get_data(&src->faces, OBZ_ObjIndex);
    if (!fbase)
      return NULL;

    /* Allocate mesh */
    OBZ_Mesh3D* out = obz_calloc(1, sizeof(OBZ_Mesh3D));

    /* Basic counts */
    out->noOfVerts = obz_arr_size(&src->positions);
    out->noOfUVs   = obz_arr_size(&src->texcoords);
    out->noOfNorms = obz_arr_size(&src->normals);
    out->noOfFaces = num_tris;

    if (out->noOfVerts)
      out->verts = memcpy(obz_malloc(sizeof(Vec3) * out->noOfVerts), src->positions.obz_data,
                          sizeof(Vec3) * out->noOfVerts);

    if (out->noOfUVs)
      out->uvs = memcpy(obz_malloc(sizeof(Vec2) * out->noOfUVs), src->texcoords.obz_data,
                        sizeof(Vec2) * out->noOfUVs);

    if (out->noOfNorms)
      out->norms = memcpy(obz_malloc(sizeof(Vec3) * out->noOfNorms), src->normals.obz_data,
                          sizeof(Vec3) * out->noOfNorms);

    /* Allocate index buffers */
    size_t idx_count  = num_tris * 3;
    out->indices      = obz_malloc(sizeof(int) * idx_count);
    out->uv_indices   = obz_malloc(sizeof(int) * idx_count);
    out->norm_indices = obz_malloc(sizeof(int) * idx_count);

    out->materials      = NULL;
    out->material_count = obz_arr_size(&src->materials);

    if (out->material_count > 0)
      out->materials = memcpy(obz_malloc(sizeof(OBZ_Material) * out->material_count),
                              src->materials.obz_data, sizeof(OBZ_Material) * out->material_count);

    out->face_mtl_id = obz_malloc(sizeof(int) * out->noOfFaces);
    memcpy(out->face_mtl_id, src->face_mtl_id.obz_data, sizeof(int) * out->noOfFaces);

    /* Fill triangles */
    for (obz_count_t tri = 0; tri < num_tris; ++tri)
    {
      int          base = tri * 3;
      OBZ_ObjIndex a    = fbase[base + 0];
      OBZ_ObjIndex b    = fbase[base + 1];
      OBZ_ObjIndex c    = fbase[base + 2];

      out->indices[base + 0] = (a.v > 0) ? a.v - 1 : -1;
      out->indices[base + 1] = (b.v > 0) ? b.v - 1 : -1;
      out->indices[base + 2] = (c.v > 0) ? c.v - 1 : -1;

      out->uv_indices[base + 0] = (a.vt > 0) ? a.vt - 1 : -1;
      out->uv_indices[base + 1] = (b.vt > 0) ? b.vt - 1 : -1;
      out->uv_indices[base + 2] = (c.vt > 0) ? c.vt - 1 : -1;

      out->norm_indices[base + 0] = (a.vn > 0) ? a.vn - 1 : -1;
      out->norm_indices[base + 1] = (b.vn > 0) ? b.vn - 1 : -1;
      out->norm_indices[base + 2] = (c.vn > 0) ? c.vn - 1 : -1;
    }

    return out;
  }

#ifdef __cplusplus
}
#endif

#endif /* OBZ_SDL_MESH_H */
