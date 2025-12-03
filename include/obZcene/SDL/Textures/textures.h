#ifndef OBZ_SDL_TEXTURE_H
#define OBZ_SDL_TEXTURE_H

#include "SDL/Colors/colors.h"
#include "obz_macros.h"
#include <SDL2/SDL_image.h>

// can change this as time goes on but 16 is enough for now
#define OBZ_MAX_MESH_TEXTURES 16
#define OBZ_MAX_MESH_FACES    128

OBZ_BEGIN_CPP_DECLS

typedef struct
{
  unsigned int id;
  int          width;
  int          height;
  OBZ_channel* pixels; // RGBA pixel data
  OBZ_Color    diffuse;
} OBZ_Texture;

OBZ_API OBZ_Texture* obz_tex_load_png(const char* path);
OBZ_API void         obz_tex_free(OBZ_Texture* tex);

OBZ_END_CPP_DECLS

#endif /* OBZ_SDL_TEXTURE_H */
