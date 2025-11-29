#include "SDL/Render/render.h"
#include <stdlib.h>

void obz_renderer_clear(OBZ_RendererContext* renctx, pixel r, pixel g, pixel b, pixel a)
{
  if (!renctx || !renctx->framebuffer)
    return;
  int n = renctx->width * renctx->height * 4;
  for (int i = 0; i < n; i += 4)
  {
    renctx->framebuffer[i + 0] = r;
    renctx->framebuffer[i + 1] = g;
    renctx->framebuffer[i + 2] = b;
    renctx->framebuffer[i + 3] = a; // alpha
  }
}

void __OBZ_core_put_pixel(OBZ_RendererContext* renctx, int x, int y, pixel r, pixel g, pixel b,
                         pixel a)
{
  if (!renctx || !renctx->framebuffer)
    return;
  if (x < 0 || x >= renctx->width || y < 0 || y >= renctx->height)
    return;

  pixel* fb = renctx->framebuffer + 4 * (y * renctx->width + x);

  fb[0] = r; // R
  fb[1] = g; // G
  fb[2] = b; // B
  fb[3] = a; // A
}

void obz_render_pixel(OBZ_RendererContext* renctx, int x, int y, OBZ_Color color)
{
  __OBZ_core_put_pixel(renctx, x, y, color.r, color.g, color.b, color.a);
}

// Simple line drawer (Bresenham)
void obz_draw_line(OBZ_RendererContext* renctx, Vec2 p0, Vec2 p1, OBZ_Color color)
{
  int x0 = (int)p0.x, y0 = (int)p0.y;
  int x1 = (int)p1.x, y1 = (int)p1.y;

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  while (1)
  {
    obz_render_pixel(renctx, x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

/* =======================================================
   Textured Mesh Renderer
   ======================================================= */
void obz_draw_mesh_textured_camera(OBZ_RendererContext* renctx, Vec3 pos, OBZ_Mesh3D* mesh,
                                   OBZ_MeshTextures* texs, OBZ_Color color, float pitch, float yaw,
                                   float roll, OBZ_Camera cam)
{
  if (!renctx || !mesh)
    return;

  int nverts = mesh->nverts;
  int nfaces = mesh->nfaces;

  // Allocate projected screen coords
  int* px = alloca(sizeof(int) * nverts);
  int* py = alloca(sizeof(int) * nverts);

  // precompute rotations
  float cp = cosf(pitch), sp = sinf(pitch);
  float cy = cosf(yaw), sy = sinf(yaw);
  float cr = cosf(roll), sr = sinf(roll);

  for (int i = 0; i < nverts; i++)
  {
    Vec3 v = mesh->verts[i];

    // Yaw
    float x1 = v.x * cy - v.z * sy;
    float z1 = v.x * sy + v.z * cy;

    // Pitch
    float y2 = v.y * cp - z1 * sp;
    float z2 = v.y * sp + z1 * cp;

    // Roll
    float x3 = x1 * cr - y2 * sr;
    float y3 = x1 * sr + y2 * cr;

    // World translate
    Vec3 world = {x3 + pos.x, y3 + pos.y, z2 + pos.z};

    obz_project_camera(world, cam, &px[i], &py[i], renctx->width, renctx->height);
  }

  for (int f = 0; f < nfaces; f++)
  {
    int* face = mesh->faces[f];

    OBZ_Texture* tex = (texs && texs->textures[0]) ? texs->textures[0] : NULL;

    // Wireframe
    for (int i = 0; i < 4; i++)
    {
      int j = (i + 1) % 4;
      if (px[face[i]] >= 0 && py[face[i]] >= 0 && px[face[j]] >= 0 && py[face[j]] >= 0)
      {
        obz_draw_line(renctx, obz_vec2(px[face[i]], py[face[i]]),
                      obz_vec2(px[face[j]], py[face[j]]), color);
      }
    }

    if (!tex)
      continue;

    // Bounding box fill
    int minx = px[face[0]], maxx = px[face[0]];
    int miny = py[face[0]], maxy = py[face[0]];
    for (int i = 1; i < 4; i++)
    {
      if (px[face[i]] < minx)
        minx = px[face[i]];
      if (px[face[i]] > maxx)
        maxx = px[face[i]];
      if (py[face[i]] < miny)
        miny = py[face[i]];
      if (py[face[i]] > maxy)
        maxy = py[face[i]];
    }

    if (minx < 0)
      minx = 0;
    if (maxx >= renctx->width)
      maxx = renctx->width - 1;
    if (miny < 0)
      miny = 0;
    if (maxy >= renctx->height)
      maxy = renctx->height - 1;

    for (int y = miny; y <= maxy; y++)
    {
      for (int x = minx; x <= maxx; x++)
      {
        float u = (float)(x - minx) / (float)(maxx - minx);
        float v = (float)(y - miny) / (float)(maxy - miny);

        int tx = (int)(u * (tex->width - 1));
        int ty = (int)(v * (tex->height - 1));

        pixel* p = tex->pixels + 4 * (ty * tex->width + tx);

        obz_render_pixel(renctx, x, y, (OBZ_Color){p[0], p[1], p[2], p[3]});
      }
    }
  }
}
