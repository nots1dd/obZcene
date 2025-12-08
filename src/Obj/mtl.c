#include "Obj/mtl.h"
#include "Utils/string.h"
#include <libgen.h>

int __OBZ_find_material(const OBZ_ObjMesh* mesh, const char* name)
{
  if (!mesh || !name)
    return -1;
  for (size_t i = 0; i < mesh->materials.size; ++i)
  {
    const OBZ_ObjMtl* m = (const OBZ_ObjMtl*)obz_arr_get_const(&mesh->materials, i);
    if (m && strcmp(m->name, name) == 0)
      return (int)i;
  }
  return -1;
}

int __OBZ_push_material(OBZ_ObjMesh* mesh, const OBZ_ObjMtl* mat)
{
  if (!mesh || !mat)
    return -1;
  obz_arr_push(&mesh->materials, mat);
  return (int)mesh->materials.size - 1;
}

OBZ_Result obz_mtl_load(const char* mtl_path, OBZ_ObjMesh* out)
{
  if (!mtl_path || !out)
    return OBZ_ERR_INVALID;
  FILE* fp = fopen(mtl_path, "r");
  if (!fp)
  {
    OBZ_LOG_WARN(NULL, "MTL: could not open '%s' (ignored)", mtl_path);
    return OBZ_ERR_FILE;
  }

  char       line[OBZ_OBJ_LINE_MAX];
  OBZ_ObjMtl cur;
  int        in_mat = 0;

  memset(&cur, 0, sizeof(cur));
  cur.Ns = 0.0f;
  cur.Kd = (Vec3f){1.0f, 1.0f, 1.0f};
  cur.Ka = (Vec3f){0.0f, 0.0f, 0.0f};
  cur.Ks = (Vec3f){0.0f, 0.0f, 0.0f};

  while (fgets(line, sizeof(line), fp))
  {
    char* p = __OBZ_skipws(line);
    if (*p == '#' || *p == '\0')
      continue;

    if (strncmp(p, "newmtl", 6) == 0 && isspace((uchar)p[6]))
    {
      if (in_mat)
      {
        __OBZ_push_material(out, &cur);
        memset(&cur, 0, sizeof(cur));
        cur.Ns = 0.0f;
        cur.Kd = (Vec3f){1, 1, 1};
      }
      char name[64] = {0};
      sscanf(p + 6, "%63s", name);
      obz_strncpy(cur.name, name, sizeof(cur.name) - 1);
      in_mat = 1;
      continue;
    }

    if (in_mat && strncmp(p, "Kd", 2) == 0 && isspace((uchar)p[2]))
    {
      sscanf(p + 2, "%f %f %f", &cur.Kd.x, &cur.Kd.y, &cur.Kd.z);
      continue;
    }
    if (in_mat && strncmp(p, "Ka", 2) == 0 && isspace((uchar)p[2]))
    {
      sscanf(p + 2, "%f %f %f", &cur.Ka.x, &cur.Ka.y, &cur.Ka.z);
      continue;
    }
    if (in_mat && strncmp(p, "Ks", 2) == 0 && isspace((uchar)p[2]))
    {
      sscanf(p + 2, "%f %f %f", &cur.Ks.x, &cur.Ks.y, &cur.Ks.z);
      continue;
    }
    if (in_mat && strncmp(p, "Ns", 2) == 0 && isspace((uchar)p[2]))
    {
      sscanf(p + 2, "%f", &cur.Ns);
      continue;
    }
    if (in_mat && strncmp(p, "map_Kd", 6) == 0 && isspace((uchar)p[6]))
    {
      char texpath_rel[256] = {0};
      sscanf(p + 6, "%255s", texpath_rel);

      char dir[PATH_MAX] = {0};
      obz_strncpy(dir, mtl_path, PATH_MAX - 1);
      char* dname = dirname(dir);

      char full_texpath[PATH_MAX] = {0};
      snprintf(full_texpath, PATH_MAX, "%s/%s", dname, texpath_rel);

      // Free any previously assigned texture
      if (cur.map_Kd)
      {
        obz_tex_free(cur.map_Kd);
        cur.map_Kd = NULL;
      }

      // Load texture from the parsed path
      cur.map_Kd = obz_tex_load_png(full_texpath);
      if (!cur.map_Kd)
      {
        OBZ_LOG_WARN(NULL, "Failed to load texture for material: %s", full_texpath);
      }

      continue;
    }
  }

  if (in_mat)
  {
    __OBZ_push_material(out, &cur);
  }

  fclose(fp);
  OBZ_LOG_INFO(NULL, "MTL: loaded %zu materials from '%s'", out->materials.size, mtl_path);
  return OBZ_OK;
}
