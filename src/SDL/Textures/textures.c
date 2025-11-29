#include "SDL/Textures/textures.h"
#include "obz_log.h"

static unsigned int g_tex_id_counter = 1;

OBZ_Texture* obz_tex_load_png(const char* path)
{
  SDL_Surface* surf = IMG_Load(path);
  if (!surf)
  {
    OBZ_LOG_FATAL(NULL, "Failed to load PNG surface '%s': %s\n", path, IMG_GetError());
    return NULL;
  }

  SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR8888, 0);
  SDL_FreeSurface(surf);

  if (!rgba)
  {
    OBZ_LOG_FATAL(NULL, "ConvertSurfaceFormat failed: %s\n", SDL_GetError());
    return NULL;
  }

  OBZ_Texture* t = malloc(sizeof(OBZ_Texture));
  if (!t)
  {
    SDL_FreeSurface(rgba);
    return NULL;
  }

  t->width  = rgba->w;
  t->height = rgba->h;
  t->id     = g_tex_id_counter++;

  size_t size = (size_t)t->width * t->height * 4;
  t->pixels   = malloc(size);

  if (!t->pixels)
  {
    free(t);
    SDL_FreeSurface(rgba);
    return NULL;
  }

  memcpy(t->pixels, rgba->pixels, size);
  SDL_FreeSurface(rgba);

  return t;
}

void obz_tex_free(OBZ_Texture* tex)
{
  if (!tex)
    return;
  free(tex->pixels);
  free(tex);
}

OBZ_MeshTextures* obz_tex_create_for_mesh(OBZ_Mesh3D* mesh)
{
  if (!mesh)
    return NULL;

  OBZ_MeshTextures* mt = calloc(1, sizeof(OBZ_MeshTextures));
  if (!mt)
    return NULL;

  mt->mesh = mesh;

  // Initialize texture slots and per-face mapping
  for (int i = 0; i < OBZ_MAX_MESH_TEXTURES; i++)
    mt->textures[i] = NULL;

  for (int i = 0; i < OBZ_MAX_MESH_FACES; i++)
    mt->face_tex_index[i] = -1; // -1 = no texture

  return mt;
}

void obz_tex_bind_to_mesh(OBZ_MeshTextures* mt, OBZ_Texture* tex, int slot)
{
  if (!mt || !tex)
    return;
  if (slot < 0 || slot >= OBZ_MAX_MESH_TEXTURES)
    return;
  mt->textures[slot] = tex;
}

void obz_tex_unbind_from_mesh(OBZ_MeshTextures* mt, int slot)
{
  if (!mt)
    return;
  if (slot < 0 || slot >= OBZ_MAX_MESH_TEXTURES)
    return;
  mt->textures[slot] = NULL;
}

void obz_tex_set_face_texture(OBZ_MeshTextures* mt, int face_index, int tex_slot)
{
  if (!mt)
    return;
  if (face_index < 0 || face_index >= OBZ_MAX_MESH_FACES)
    return;
  if (tex_slot < 0 || tex_slot >= OBZ_MAX_MESH_TEXTURES)
    return;
  mt->face_tex_index[face_index] = tex_slot;
}

OBZ_Texture* obz_tex_get_face_texture(OBZ_MeshTextures* mt, int face_index)
{
  if (!mt)
    return NULL;
  if (face_index < 0 || face_index >= OBZ_MAX_MESH_FACES)
    return NULL;

  int slot = mt->face_tex_index[face_index];
  if (slot < 0 || slot >= OBZ_MAX_MESH_TEXTURES)
    return NULL;

  return mt->textures[slot];
}
