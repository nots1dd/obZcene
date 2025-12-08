#include "SDL/Mesh/mesh.h"
#include "Utils/null.h"

void obz_mesh_free_all(OBZ_Mesh3D* mesh)
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

OBZ_Mesh3D* obz_mesh_from_obj(const OBZ_ObjMesh* src)
{
  __OBZ_RETURN_NULL_IF_NULL(src);

  auto num_idx_entries = (int)src->faces.size;
  if (num_idx_entries <= 0)
    return NULL;

  // ---------------------- INDEX COUNT -> TRIANGLE COUNT ------------------------
  /*
      OBJ files store faces as sets of vertex-index triplets: (v, vt, vn).
      Every *3* OBJ face-index entries define a single triangle.

          num_tris = index_entry_count / 3

      Simple integer division – no geometry here, but it establishes the
      size of triangle-based buffers.
  */
  // ----------------------------------------------------------------------------
  obz_count_t num_tris = num_idx_entries / 3;
  auto        fbase    = obz_arr_get_data(&src->faces, OBZ_ObjIndex);

  __OBZ_RETURN_NULL_IF_NULL(fbase);

  /* Allocate mesh */
  OBZ_Mesh3D* out_mesh = obz_calloc(1, sizeof(OBZ_Mesh3D));

  /* Basic counts */
  out_mesh->noOfVerts = obz_arr_size(&src->positions);
  out_mesh->noOfUVs   = obz_arr_size(&src->texcoords);
  out_mesh->noOfNorms = obz_arr_size(&src->normals);
  out_mesh->noOfFaces = num_tris;

  if (out_mesh->noOfVerts)
    out_mesh->verts =
      memcpy(obz_malloc(sizeof(Vec3d) * out_mesh->noOfVerts),
             (Vec3d*)obz_arr_get_data_const(&src->positions), sizeof(Vec3d) * out_mesh->noOfVerts);

  if (out_mesh->noOfUVs)
    out_mesh->uvs =
      memcpy(obz_malloc(sizeof(Vec2d) * out_mesh->noOfUVs),
             (Vec2d*)obz_arr_get_data_const(&src->texcoords), sizeof(Vec2d) * out_mesh->noOfUVs);

  if (out_mesh->noOfNorms)
    out_mesh->norms =
      memcpy(obz_malloc(sizeof(Vec3d) * out_mesh->noOfNorms),
             (Vec3d*)obz_arr_get_data_const(&src->normals), sizeof(Vec3d) * out_mesh->noOfNorms);

  /* Allocate index buffers */
  const obz_index_t idx_count = num_tris * 3;
  out_mesh->indices           = obz_malloc(sizeof(int) * idx_count);
  out_mesh->uv_indices        = obz_malloc(sizeof(int) * idx_count);
  out_mesh->norm_indices      = obz_malloc(sizeof(int) * idx_count);

  out_mesh->materials      = NULL;
  out_mesh->material_count = obz_arr_size(&src->materials);

  if (out_mesh->material_count > 0)
    out_mesh->materials = memcpy(obz_malloc(sizeof(OBZ_ObjMtl) * out_mesh->material_count),
                                 (OBZ_ObjMtl*)obz_arr_get_data_const(&src->materials),
                                 sizeof(OBZ_ObjMtl) * out_mesh->material_count);

  out_mesh->face_mtl_id = obz_malloc(sizeof(int) * out_mesh->noOfFaces);
  memcpy(out_mesh->face_mtl_id, (int*)obz_arr_get_data_const(&src->face_mtl_id),
         sizeof(int) * out_mesh->noOfFaces);

  /* Fill triangles */
  for (obz_count_t tri = 0; tri < num_tris; ++tri)
  {
    const int    base = tri * 3;
    OBZ_ObjIndex a    = fbase[base + 0];
    OBZ_ObjIndex b    = fbase[base + 1];
    OBZ_ObjIndex c    = fbase[base + 2];

    // ---------------------- OBJ INDICES -> 0-BASED ------------------------
    /*
        OBJ uses 1-based indices: (1 ... N).
        Internal representation uses 0-based indices: (0 ... N-1).

        Conversion:   internal = obj - 1
        If the OBJ index is 0 or negative (rare but permitted), we mark as -1.
    */
    // ----------------------------------------------------------------------
    out_mesh->indices[base + 0] = (a.v > 0) ? a.v - 1 : -1;
    out_mesh->indices[base + 1] = (b.v > 0) ? b.v - 1 : -1;
    out_mesh->indices[base + 2] = (c.v > 0) ? c.v - 1 : -1;

    out_mesh->uv_indices[base + 0] = (a.vt > 0) ? a.vt - 1 : -1;
    out_mesh->uv_indices[base + 1] = (b.vt > 0) ? b.vt - 1 : -1;
    out_mesh->uv_indices[base + 2] = (c.vt > 0) ? c.vt - 1 : -1;

    out_mesh->norm_indices[base + 0] = (a.vn > 0) ? a.vn - 1 : -1;
    out_mesh->norm_indices[base + 1] = (b.vn > 0) ? b.vn - 1 : -1;
    out_mesh->norm_indices[base + 2] = (c.vn > 0) ? c.vn - 1 : -1;
  }

  return out_mesh;
}
