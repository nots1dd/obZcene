#ifndef OBZ_SDL_MESH_H
#define OBZ_SDL_MESH_H

#include "Utils/vec.h"
#include <stdlib.h>

// 3d stuff
typedef struct
{
  Vec3* verts;
  int   nverts;
  int (*faces)[4]; // array of quads
  int   nfaces;
  Vec2* uvs;
} OBZ_Mesh3D;

/* ------------ FACE ALLOCATOR ------------ */
OBZ_FORCE_INLINE static int (*alloc_faces(int nfaces))[4]
{
  return malloc(sizeof(int[4]) * nfaces);
}

inline static void free_mesh(OBZ_Mesh3D* m)
{
  free(m->verts);
  free(m->faces);
  m->verts  = NULL;
  m->faces  = NULL;
  m->nverts = m->nfaces = 0;
}

inline static OBZ_Mesh3D make_wire_plane(float w, float h, int tilesX, int tilesY)
{
  OBZ_Mesh3D m = {0};

  if (tilesX <= 0)
    tilesX = 1;
  if (tilesY <= 0)
    tilesY = 1;

  int vertsX = tilesX + 1;
  int vertsY = tilesY + 1;

  /* verts */
  m.nverts = vertsX * vertsY;
  m.verts  = malloc(sizeof(Vec3) * m.nverts);
  if (!m.verts)
  {
    m.nverts = 0;
    return m;
  }

  float dx  = w / tilesX;
  float dy  = h / tilesY;
  int   idx = 0;
  for (int iy = 0; iy < vertsY; iy++)
  {
    float y = -h * 0.5f + iy * dy;
    for (int ix = 0; ix < vertsX; ix++)
    {
      float x        = -w * 0.5f + ix * dx;
      m.verts[idx++] = (Vec3){x, y, 0.0f};
    }
  }

  /* faces (quads) */
  m.nfaces = tilesX * tilesY;
  m.faces  = malloc(sizeof(int[4]) * m.nfaces);
  if (!m.faces)
  {
    free(m.verts);
    m.verts  = NULL;
    m.nverts = 0;
    m.nfaces = 0;
    return m;
  }

  int f = 0;
  for (int iy = 0; iy < tilesY; iy++)
  {
    for (int ix = 0; ix < tilesX; ix++)
    {
      int v0        = iy * vertsX + ix;
      int v1        = v0 + 1;
      int v2        = v0 + vertsX + 1;
      int v3        = v0 + vertsX;
      m.faces[f][0] = v0;
      m.faces[f][1] = v1;
      m.faces[f][2] = v2;
      m.faces[f][3] = v3;
      f++;
    }
  }

  return m;
}

#endif
