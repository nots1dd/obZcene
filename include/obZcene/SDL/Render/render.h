#ifndef OBZ_RENDER_H
#define OBZ_RENDER_H

#include "SDL/Mesh/3dshapes.h"
#include "SDL/core.h"
#include "SDL/shapes.h"

// this file is a goddamn misnomer; only generates edges there is no rasterization or textures being created yet.

typedef struct
{
  float diffuse[3];
  float specular[3];
  float shininess;
} OBZ_Material;

/* -------------- High-performance version with clipping -------------- */
inline static void draw_mesh_wire_camera_fast(OBZ_Context* ctx, Vec3 pos, OBZ_Mesh3D* m,
                                              float pitch, float yaw, float roll,
                                              OBZ_Color c, OBZ_Camera cam)
{
  const int count    = m->nverts;
  const int use_heap = count > 1024;
  const int sw = ctx->win_desc->width;
  const int sh = ctx->win_desc->height;

  int* px = use_heap ? malloc(sizeof(int) * count) : alloca(sizeof(int) * count);
  int* py = use_heap ? malloc(sizeof(int) * count) : alloca(sizeof(int) * count);

  /* --- Precompute rotation --- */
  float cp = cosf(pitch), sp = sinf(pitch);
  float cy = cosf(yaw), sy = sinf(yaw);
  float cr = cosf(roll), sr = sinf(roll);

  /* --- Transform all vertices --- */
  for (int i = 0; i < count; i++)
  {
    Vec3 v = m->verts[i];

    // Yaw
    float x1 = v.x * cy - v.z * sy;
    float z1 = v.x * sy + v.z * cy;

    // Pitch
    float y2 = v.y * cp - z1 * sp;
    float z2 = v.y * sp + z1 * cp;

    // Roll
    float x3 = x1 * cr - y2 * sr;
    float y3 = x1 * sr + y2 * cr;

    // Translate to world space
    Vec3 world = {x3 + pos.x, y3 + pos.y, z2 + pos.z};

    // Project
    obz_project_camera(world, cam, &px[i], &py[i], sw, sh);
  }

  /* --- Draw wireframe edges --- */
  for (int f = 0; f < m->nfaces; f++)
  {
    int* face = m->faces[f];

    for (int j = 0; j < 4; j++)
    {
      int a = face[j];
      int b = face[(j + 1) % 4];

      int ax = px[a], ay = py[a];
      int bx = px[b], by = py[b];

      // Simple trivial reject
      if (ax < 0 || ay < 0 || ax >= sw || ay >= sh)
        continue;
      if (bx < 0 || by < 0 || bx >= sw || by >= sh)
        continue;

      obz_draw_line(ctx, (OBZ_Point){ax, ay}, (OBZ_Point){bx, by}, c);
    }
  }

  if (use_heap)
  {
    free(px);
    free(py);
  }
}

#endif
