#ifndef OBZCENE_H
#define OBZCENE_H

#include "SDL/Mesh/3dshapes.h"
#include "SDL/Render/render.h"
#include "SDL/Scene/scene.h"
#include "SDL/Textures/textures.h"
#include "obz_log.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const float W      = 1000;
static const float H      = 400;
static const float D      = 1000;
static const float margin = 20.0f;

static OBZ_Mesh3D* sphere  = NULL;
static OBZ_Mesh3D* pyramid = NULL;
static OBZ_Mesh3D* quad    = NULL;

/* Textures */
static OBZ_Texture* quad_tex     = NULL;
static OBZ_Texture* pyramid_tex1 = NULL;
static OBZ_Texture* pyramid_tex2 = NULL;
static OBZ_Texture* pyramid_tex3 = NULL;
static OBZ_Texture* pyramid_tex4 = NULL;
static OBZ_Texture* sphere_tex   = NULL;

/* corner pyramids */
static const Vec3 pyramid_pos1 = {400, -150, -400};
static const Vec3 pyramid_pos2 = {400, -150, 400};
static const Vec3 pyramid_pos3 = {-400, -150, 400};
static const Vec3 pyramid_pos4 = {-400, -150, -400};

inline static void draw_crosshair(OBZ_RendererContext* rc, int cx, int cy, int length, int gap,
                                  OBZ_pixel color)
{
  // horizontal left
  for (int x = cx - length; x < cx - gap; x++)
    obz_render_pixel(rc, x, cy, 0, color);

  // horizontal right
  for (int x = cx + gap; x <= cx + length; x++)
    obz_render_pixel(rc, x, cy, 0, color);

  // vertical top
  for (int y = cy - length; y < cy - gap; y++)
    obz_render_pixel(rc, cx, y, 0, color);

  // vertical bottom
  for (int y = cy + gap; y <= cy + length; y++)
    obz_render_pixel(rc, cx, y, 0, color);
}

// idk what the fuck this is (disco ball)
inline static OBZ_Texture* make_circle_texture(int size, OBZ_Color inner, OBZ_Color ring,
                                               OBZ_Color bg)
{
  if (size <= 0)
    return NULL;
  OBZ_Texture* tex = malloc(sizeof(OBZ_Texture));
  if (!tex)
    return NULL;
  tex->width  = size;
  tex->height = size;
  tex->pixels = malloc((size_t)size * size * 4);
  if (!tex->pixels)
  {
    free(tex);
    return NULL;
  }

  float cx     = (size - 1) * 0.5f;
  float cy     = (size - 1) * 0.5f;
  float radius = (size - 1) * 0.5f;

  for (int y = 0; y < size; ++y)
  {
    for (int x = 0; x < size; ++x)
    {
      int   idx = 4 * (y * size + x);
      float dx  = x - cx;
      float dy  = y - cy;
      float d   = OBZ__SIMD_sqrt_scalar_sse(dx * dx + dy * dy) / radius; // normalized 0..~1.4

      OBZ_Color c;
      if (d <= 0.55f)
      {
        c = inner;
      }
      else if (d <= 0.85f)
      {
        /* interpolate between inner and ring for a smoother band */
        float t = (d - 0.55f) / (0.30f);
        c.r     = (OBZ_pixel)((1.0f - t) * inner.r + t * ring.r);
        c.g     = (OBZ_pixel)((1.0f - t) * inner.g + t * ring.g);
        c.b     = (OBZ_pixel)((1.0f - t) * inner.b + t * ring.b);
        c.a     = (OBZ_pixel)((1.0f - t) * inner.a + t * ring.a);
      }
      else
      {
        c = bg;
      }

      tex->pixels[idx + 0] = c.a;
      tex->pixels[idx + 1] = c.b;
      tex->pixels[idx + 2] = c.g;
      tex->pixels[idx + 3] = c.r;
    }
  }

  return tex;
}

/* standard checkerboard */
inline static OBZ_Texture* make_checker_texture(int size, OBZ_Color a, OBZ_Color b, int squares)
{
  if (size <= 0 || squares <= 0)
    return NULL;
  OBZ_Texture* tex = malloc(sizeof(OBZ_Texture));
  if (!tex)
    return NULL;
  tex->width  = size;
  tex->height = size;
  tex->pixels = malloc((size_t)size * size * 4);
  if (!tex->pixels)
  {
    free(tex);
    return NULL;
  }

  int sq = size / squares;
  if (sq <= 0)
    sq = 1;

  for (int y = 0; y < size; ++y)
  {
    for (int x = 0; x < size; ++x)
    {
      int       xi         = x / sq;
      int       yi         = y / sq;
      OBZ_Color c          = ((xi + yi) & 1) ? a : b;
      int       idx        = 4 * (y * size + x);
      tex->pixels[idx + 0] = c.a;
      tex->pixels[idx + 1] = c.b;
      tex->pixels[idx + 2] = c.g;
      tex->pixels[idx + 3] = c.r;
    }
  }

  return tex;
}

inline static OBZ_Texture* make_wall_border_texture(int w, int h, OBZ_Color wall_col,
                                                    OBZ_Color border_col, int thickness)
{
  if (w <= 0 || h <= 0)
    return NULL;
  OBZ_Texture* tex = malloc(sizeof(OBZ_Texture));
  if (!tex)
    return NULL;
  tex->width  = w;
  tex->height = h;
  tex->pixels = malloc((size_t)w * h * 4);
  if (!tex->pixels)
  {
    free(tex);
    return NULL;
  }

  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      int       idx = 4 * (y * w + x);
      OBZ_Color c   = wall_col;
      if (x < thickness || x >= w - thickness || y < thickness || y >= h - thickness)
        c = border_col;
      tex->pixels[idx + 0] = c.a;
      tex->pixels[idx + 1] = c.b;
      tex->pixels[idx + 2] = c.g;
      tex->pixels[idx + 3] = c.r;
    }
  }

  return tex;
}

/* ----------------- Helper to map textures to faces ----------------- */
static inline void bind_texture_and_map_all_faces(OBZ_Scene* scene, int mesh_id, OBZ_Mesh3D* mesh,
                                                  OBZ_Texture* tex, int slot)
{
  if (!scene || mesh_id < 0 || !mesh || !tex)
    return;

  obz_scene_set_texture(scene, mesh_id, tex, slot); /* this will create & bind if needed */

  OBZ_MeshTextures* mt = obz_scene_get_mesh_textures(scene, mesh_id);
  if (!mt)
  {
    OBZ_LOG_WARN(NULL, "bind_texture_and_map_all_faces: could not get mesh_textures for id %d",
                 mesh_id);
    return;
  }

  /* map every face -> slot */
  for (int f = 0; f < mesh->nfaces; ++f)
    obz_tex_set_face_texture(mt, f, slot);
}

inline static void scene_add_room(OBZ_Scene* scene, float W, float H, float D, int tiles,
                                  Rot3 speed)
{
  float       floorY = -H * 0.5f;
  float       ceilY  = H * 0.5f;
  OBZ_Mesh3D* plane  = NULL;

  OBZ_Color    wall_col   = COLOR_LIGHT_GRAY;
  OBZ_Color    border_col = COLOR_DARK_GRAY;
  OBZ_Texture* wall_tex   = make_wall_border_texture(64, 64, wall_col, border_col, 2);
  if (!wall_tex)
    OBZ_LOG_WARN(NULL, "make_wall_border_texture failed");

  /* FLOOR */
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, D, tiles, tiles);
  int pid =
    obz_scene_add_mesh(scene, plane, (Vec3){0, floorY, 0}, (Rot3){-M_PI / 2, 0, 0}, 1, speed);
  if (wall_tex)
    bind_texture_and_map_all_faces(scene, pid, plane, wall_tex, 0);

  /* CEILING */
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, D, tiles, tiles);
  pid    = obz_scene_add_mesh(scene, plane, (Vec3){0, ceilY, 0}, (Rot3){M_PI / 2, 0, 0}, 1, speed);

  if (wall_tex)
    bind_texture_and_map_all_faces(scene, pid, plane, wall_tex, 0);

  /* BACK */
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, H, tiles, tiles);
  pid    = obz_scene_add_mesh(scene, plane, (Vec3){0, 0, -D / 2}, (Rot3){0, 0, 0}, 1, speed);
  if (wall_tex)
    bind_texture_and_map_all_faces(scene, pid, plane, wall_tex, 0);

  /* FRONT */
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(W, H, tiles, tiles);
  pid    = obz_scene_add_mesh(scene, plane, (Vec3){0, 0, D / 2}, (Rot3){0, M_PI, 0}, 1, speed);
  if (wall_tex)
    bind_texture_and_map_all_faces(scene, pid, plane, wall_tex, 0);

  /* LEFT */
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(D, H, tiles, tiles);
  pid = obz_scene_add_mesh(scene, plane, (Vec3){-W / 2, 0, 0}, (Rot3){0, -M_PI / 2, 0}, 1, speed);
  if (wall_tex)
    bind_texture_and_map_all_faces(scene, pid, plane, wall_tex, 0);

  /* RIGHT */
  plane  = malloc(sizeof(OBZ_Mesh3D));
  *plane = make_wire_plane(D, H, tiles, tiles);
  pid    = obz_scene_add_mesh(scene, plane, (Vec3){W / 2, 0, 0}, (Rot3){0, M_PI / 2, 0}, 1, speed);
  if (wall_tex)
    bind_texture_and_map_all_faces(scene, pid, plane, wall_tex, 0);
}

inline static void init_scene(OBZ_Scene* scene)
{
  if (!scene)
  {
    OBZ_LOG_WARN(NULL, "init_scene: scene is NULL");
    return;
  }

  sphere   = malloc(sizeof(OBZ_Mesh3D));
  *sphere  = make_sphere(50, 12);
  pyramid  = malloc(sizeof(OBZ_Mesh3D));
  *pyramid = make_pyramid(80, 120);
  quad     = malloc(sizeof(OBZ_Mesh3D));
  *quad    = make_quad(30, 30);

  scene_add_room(scene, W, H, D, 4, (Rot3){0, 0, 0});

  /* pyramids: create checker textures and bind per-mesh */
  pyramid_tex1 = make_checker_texture(64, COLOR_BLUE, COLOR_GREEN, 8);
  pyramid_tex2 = make_checker_texture(64, COLOR_ORANGE, COLOR_RED, 8);
  pyramid_tex3 = make_checker_texture(64, COLOR_PURPLE, COLOR_YELLOW, 8);
  pyramid_tex4 = make_checker_texture(64, COLOR_WHITE, COLOR_BLACK, 8);

  int pid1 = obz_scene_add_mesh(scene, pyramid, pyramid_pos1, (Rot3){0, 0, 0}, 1, (Rot3){0, 10, 0});
  if (pyramid_tex1)
    bind_texture_and_map_all_faces(scene, pid1, pyramid, pyramid_tex1, 0);

  int pid2 = obz_scene_add_mesh(scene, pyramid, pyramid_pos2, (Rot3){0, 0, 0}, 1, (Rot3){0, 10, 0});
  if (pyramid_tex2)
    bind_texture_and_map_all_faces(scene, pid2, pyramid, pyramid_tex2, 0);

  int pid3 = obz_scene_add_mesh(scene, pyramid, pyramid_pos3, (Rot3){0, 0, 0}, 1, (Rot3){0, 10, 0});
  if (pyramid_tex3)
    bind_texture_and_map_all_faces(scene, pid3, pyramid, pyramid_tex3, 0);

  int pid4 = obz_scene_add_mesh(scene, pyramid, pyramid_pos4, (Rot3){0, 0, 0}, 1, (Rot3){0, 10, 0});
  if (pyramid_tex4)
    bind_texture_and_map_all_faces(scene, pid4, pyramid, pyramid_tex4, 0);

  OBZ_LOG_TRACE(NULL, "Added pyramids with unique textures");

  /* Sphere with ring/circle texture */
  sphere_tex = make_circle_texture(128, COLOR_WHITE, COLOR_RED, COLOR_GOLD);
  int sid =
    obz_scene_add_mesh(scene, sphere, (Vec3){200, 60, 0}, (Rot3){0, 0, 0}, 1, (Rot3){0, 60, 0});
  if (sphere_tex)
    bind_texture_and_map_all_faces(scene, sid, sphere, sphere_tex, 0);

  OBZ_LOG_TRACE(NULL, "Added sphere with procedural texture");

  /* Quad with PNG (image) texture (color is messed up and since this is a quad, the image is split in 2 triangles) */
  int qid =
    obz_scene_add_mesh(scene, quad, (Vec3){100, 100, 0}, (Rot3){0, 0, 0}, 1, (Rot3){0, 0, 0});
  quad_tex = obz_tex_load_png("assets/obzen.png");
  if (quad_tex)
  {
    bind_texture_and_map_all_faces(scene, qid, quad, quad_tex, 0);
    OBZ_LOG_TRACE(NULL, "Quad PNG texture bound");
  }
  else
  {
    OBZ_LOG_WARN(NULL, "Could not load quad PNG texture 'assets/obzen.png'");
  }

  OBZ_LOG_TRACE(NULL, "Scene initialized with textures");
}

#endif /* OBZCENE_H */
