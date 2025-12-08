#include "Obj/parser.h"
#include "Obj/mtl.h"
#include "Utils/string.h"
#include <ctype.h>

void obz_objmesh_init(OBZ_ObjMesh* objmesh)
{
  objmesh->positions = obz_arr_create(sizeof(Vec3f));
  objmesh->texcoords = obz_arr_create(sizeof(Vec2f));
  objmesh->normals   = obz_arr_create(sizeof(Vec3f));
  objmesh->faces     = obz_arr_create(sizeof(OBZ_ObjIndex));

  objmesh->face_mtl_id = obz_arr_create(sizeof(int));
  objmesh->materials   = obz_arr_create(sizeof(OBZ_ObjMtl));

  obz_arr_reserve(&objmesh->positions, 1024);
  obz_arr_reserve(&objmesh->texcoords, 1024);
  obz_arr_reserve(&objmesh->normals, 1024);
  obz_arr_reserve(&objmesh->faces, 2048);

  obz_arr_reserve(&objmesh->face_mtl_id, 2048 / 3);
  obz_arr_reserve(&objmesh->materials, 16);
}

void obz_objmesh_free(OBZ_ObjMesh* objmesh)
{
  if (!objmesh)
    return;
  obz_arr_free(&objmesh->positions);
  obz_arr_free(&objmesh->texcoords);
  obz_arr_free(&objmesh->normals);
  obz_arr_free(&objmesh->faces);
  obz_arr_free(&objmesh->face_mtl_id);
  obz_arr_free(&objmesh->materials);
}

OBZ_ObjIndex __OBZ_parse_index(const char** s)
{
  OBZ_ObjIndex idx = {0, 0, 0};
  idx.v            = __OBZ_atoi_ptr(s);
  if (**s != '/')
    return idx;
  (*s)++;
  if (**s != '/')
    idx.vt = __OBZ_atoi_ptr(s);
  if (**s != '/')
    return idx;
  (*s)++;
  idx.vn = __OBZ_atoi_ptr(s);
  return idx;
}

int __OBZ_triangulate_and_push(OBZ_DynArray* faces, OBZ_ObjIndex* tmp, int count)
{
  int tris = 0;
  for (int i = 1; i < count - 1; ++i)
  {
    obz_arr_push(faces, &tmp[0]);
    obz_arr_push(faces, &tmp[i]);
    obz_arr_push(faces, &tmp[i + 1]);
    tris++;
  }
  return tris;
}

OBZ_Result obz_obj_load(const char* path, OBZ_ObjMesh* out)
{
  if (!path || !out)
    return OBZ_ERR_INVALID;
  FILE* fp = fopen(path, "r");
  if (!fp)
  {
    OBZ_LOG_ERROR(NULL, "OBJ: Failed to open '%s'", path);
    return OBZ_ERR_FILE;
  }

  obz_objmesh_init(out);

  Vec3f v;
  Vec2f t;
  Vec3f n;
  char  line[OBZ_OBJ_LINE_MAX];

  int   current_mtl = -1;                        /* index into out->materials, -1 = none */
  char* obj_dir     = __OBZ_dirname_alloc(path); /* for resolving mtllib/map_Kd */

  while (fgets(line, sizeof(line), fp))
  {
    char* p = __OBZ_skipws(line);
    if (*p == '#' || *p == '\0')
      continue;

    /* mtllib -> load referenced mtl file (relative to obj path) */
    if (strncmp(p, "mtllib", 6) == 0 && isspace((uchar)p[6]))
    {
      char mtlname[512] = {0};
      sscanf(p + 6, "%511s", mtlname);

      /* attempt to open as given first; if that fails, try relative to obj dir */
      char* candidate = strdup(mtlname);
      FILE* tf        = fopen(candidate, "r");
      if (!tf)
      {
        free(candidate);
        candidate = __OBZ_join_path(obj_dir, mtlname);
        tf        = candidate ? fopen(candidate, "r") : NULL;
      }
      if (tf)
      {
        fclose(tf);
        obz_mtl_load(candidate, out);
      }
      else
      {
        OBZ_LOG_ERROR(NULL, "OBJ: could not find mtl '%s' (searched relative to OBJ dir)", mtlname);
      }
      free(candidate);
      continue;
    }

    /* vertex pos */
    if (p[0] == 'v' && isspace((uchar)p[1]))
    {
      sscanf(p + 1, "%f %f %f", &v.x, &v.y, &v.z);
      obz_arr_push(&out->positions, &v);
      continue;
    }

    /* texcoord */
    if (p[0] == 'v' && p[1] == 't' && isspace((uchar)p[2]))
    {
      sscanf(p + 2, "%f %f", &t.x, &t.y);
      obz_arr_push(&out->texcoords, &t);
      continue;
    }

    /* normal */
    if (p[0] == 'v' && p[1] == 'n' && isspace((uchar)p[2]))
    {
      sscanf(p + 2, "%f %f %f", &n.x, &n.y, &n.z);
      obz_arr_push(&out->normals, &n);
      continue;
    }

    /* usemtl -> change current material */
    if (strncmp(p, "usemtl", 6) == 0 && isspace((uchar)p[6]))
    {
      char mname[64] = {0};
      sscanf(p + 6, "%63s", mname);
      int mid = __OBZ_find_material(out, mname);
      if (mid < 0)
      {
        OBZ_LOG_ERROR(NULL, "OBJ: usemtl '%s' not found in loaded materials", mname);
      }
      current_mtl = mid;
      continue;
    }

    /* face */
    if (p[0] == 'f' && isspace((uchar)p[1]))
    {
      const char*  s = p + 1;
      OBZ_ObjIndex tmp[64];
      int          count = 0;

      s = __OBZ_skipws((char*)s);
      while (*s && count < (int)(sizeof(tmp) / sizeof(tmp[0])))
      {
        tmp[count++] = __OBZ_parse_index(&s);
        s            = __OBZ_skipws((char*)s);
      }

      OBZ_ASSERT(count >= 3, "face has fewer than 3 vertices");
      /* triangulate into OBJ faces array; __OBZ_triangulate_and_push returns tri count */
      int tris = __OBZ_triangulate_and_push(&out->faces, tmp, count);

      /* for each produced triangle push the current material id (can be -1) */
      for (int i = 0; i < tris; ++i)
        obz_arr_push(&out->face_mtl_id, &current_mtl);

      continue;
    }
  }

  obz_free(obj_dir);
  fclose(fp);

  OBZ_LOG_INFO(NULL, "OBJ loaded: %zu verts, %zu uvs, %zu norms, %zu triangles, %zu materials",
               out->positions.size, out->texcoords.size, out->normals.size, out->faces.size / 3,
               out->materials.size);

  OBZ_ASSERT(out->positions.size > 0, "No vertices parsed");
  OBZ_ASSERT(out->faces.size >= 3, "Not enough faces parsed");

  return OBZ_OK;
}
