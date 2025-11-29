#ifndef OBZ_TEXTURE_H
#define OBZ_TEXTURE_H

#include "SDL/Mesh/mesh.h"
#include "types.h"
#include <SDL2/SDL_image.h>

// can change this as time goes on but 16 is enough for now
#define OBZ_MAX_MESH_TEXTURES 16

typedef struct
{
  unsigned int id;
  int          width;
  int          height;
  pixel*       pixels; // RGBA pixel data
} OBZ_Texture;

typedef struct
{
  OBZ_Mesh3D*  mesh; // Mesh associated
  OBZ_Texture* textures[OBZ_MAX_MESH_TEXTURES];
} OBZ_MeshTextures;

OBZ_API OBZ_Texture*      obz_tex_load_png(const char* path);
OBZ_API OBZ_MeshTextures* obz_tex_create_for_mesh(OBZ_Mesh3D* mesh);

OBZ_API void obz_tex_free(OBZ_Texture* tex);
OBZ_API void obz_tex_bind_to_mesh(OBZ_MeshTextures* mt, OBZ_Texture* tex, int slot);
OBZ_API void obz_tex_unbind_from_mesh(OBZ_MeshTextures* mt, int slot);

#endif
