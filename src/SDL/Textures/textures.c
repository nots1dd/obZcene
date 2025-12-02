#include "SDL/Textures/textures.h"
#include "obz_mem.h"

static unsigned int g_tex_id_counter = 1;

OBZ_Texture* obz_tex_load_png(const char* path)
{
  SDL_Surface* surf = IMG_Load(path);
  if (!surf)
    return NULL;

  SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR8888, 0);
  SDL_FreeSurface(surf);

  if (!rgba)
    return NULL;

  OBZ_Texture* t = obz_calloc(1, sizeof(OBZ_Texture));
  if (!t)
  {
    SDL_FreeSurface(rgba);
    return NULL;
  }

  t->width   = rgba->w;
  t->height  = rgba->h;
  t->id      = g_tex_id_counter++;
  t->diffuse = COLOR_WHITE;

  size_t size = (size_t)t->width * t->height * 4;

  t->pixels = obz_malloc(size);
  if (!t->pixels)
  {
    SDL_FreeSurface(rgba);
    obz_free(t);
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
  obz_free(tex->pixels);
  obz_free(tex);
}
