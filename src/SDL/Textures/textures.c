#include "SDL/Textures/textures.h"

static unsigned int g_tex_id_counter = 1;

OBZ_Texture* obz_tex_load_png(const char* path)
{
  SDL_Surface* surf = IMG_Load(path);
  if (!surf)
  {
    printf("Failed to load PNG surface '%s': %s\n", path, IMG_GetError());
    return NULL;
  }

  SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surf);

  if (!rgba)
  {
    printf("ConvertSurfaceFormat failed: %s\n", SDL_GetError());
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
  OBZ_MeshTextures* mt = malloc(sizeof(OBZ_MeshTextures));
  if (!mt)
    return NULL;

  mt->mesh = mesh;
  memset(mt->textures, 0, sizeof(mt->textures));
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
