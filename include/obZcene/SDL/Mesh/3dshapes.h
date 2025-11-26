#ifndef OBZ_SDL_MESH_3DSHAPES_H
#define OBZ_SDL_MESH_3DSHAPES_H

#include "Utils/vec.h"
#include "macros.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

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

inline static OBZ_Mesh3D make_quad(float w, float h)
{
  OBZ_Mesh3D m;

  m.nverts = 4;
  m.verts  = malloc(sizeof(Vec3) * 4);

  float hw = w * 0.5f;
  float hh = h * 0.5f;

  m.verts[0] = (Vec3){-hw, -hh, 0};
  m.verts[1] = (Vec3){hw, -hh, 0};
  m.verts[2] = (Vec3){hw, hh, 0};
  m.verts[3] = (Vec3){-hw, hh, 0};

  m.nfaces = 2;
  m.faces  = alloc_faces(m.nfaces);

  static int F[2][4] = {{0, 1, 2, 3}, {0, 2, 3, 3}};

  memcpy(m.faces, F, sizeof(F));

  return m;
}

/* ---------------- Cube generator ---------------- */
inline static OBZ_Mesh3D make_cube(float size)
{
  OBZ_Mesh3D m;
  m.nverts = 8;
  m.nfaces = 6;

  m.verts = malloc(sizeof(Vec3) * m.nverts);
  m.faces = alloc_faces(m.nfaces);

  float h = size / 2;

  m.verts[0] = (Vec3){-h, -h, -h};
  m.verts[1] = (Vec3){h, -h, -h};
  m.verts[2] = (Vec3){h, h, -h};
  m.verts[3] = (Vec3){-h, h, -h};
  m.verts[4] = (Vec3){-h, -h, h};
  m.verts[5] = (Vec3){h, -h, h};
  m.verts[6] = (Vec3){h, h, h};
  m.verts[7] = (Vec3){-h, h, h};

  static int F[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
                        {2, 3, 7, 6}, {0, 3, 7, 4}, {1, 2, 6, 5}};
  memcpy(m.faces, F, sizeof(F));
  return m;
}

inline static OBZ_Mesh3D make_pyramid(float size, float height)
{
  OBZ_Mesh3D m;
  m.nverts = 5;
  m.nfaces = 5;

  m.verts = malloc(sizeof(Vec3) * m.nverts);
  m.faces = alloc_faces(m.nfaces);

  float h = size / 2;

  m.verts[0] = (Vec3){-h, 0, -h};
  m.verts[1] = (Vec3){h, 0, -h};
  m.verts[2] = (Vec3){h, 0, h};
  m.verts[3] = (Vec3){-h, 0, h};
  m.verts[4] = (Vec3){0, height, 0};

  static int F[5][4] = {{0, 1, 2, 3}, {0, 1, 4, 4}, {1, 2, 4, 4}, {2, 3, 4, 4}, {3, 0, 4, 4}};
  memcpy(m.faces, F, sizeof(F));
  return m;
}

inline static OBZ_Mesh3D make_sphere(float r, int seg)
{
  int nverts = (seg + 1) * (seg + 1);
  int nfaces = seg * seg;

  OBZ_Mesh3D m;
  m.nverts = nverts;
  m.nfaces = nfaces;
  m.verts  = malloc(sizeof(Vec3) * nverts);
  m.faces  = alloc_faces(nfaces);

  int idx = 0;
  for (int i = 0; i <= seg; i++)
  {
    float t = (float)i / seg * M_PI;
    for (int j = 0; j <= seg; j++)
    {
      float p        = (float)j / seg * 2 * M_PI;
      m.verts[idx++] = (Vec3){r * sinf(t) * cosf(p), r * cosf(t), r * sinf(t) * sinf(p)};
    }
  }

  int f = 0;
  for (int i = 0; i < seg; i++)
  {
    for (int j = 0; j < seg; j++)
    {
      int v0        = i * (seg + 1) + j;
      int v1        = v0 + 1;
      int v2        = v0 + (seg + 1) + 1;
      int v3        = v0 + (seg + 1);
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
