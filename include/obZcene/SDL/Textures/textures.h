#ifndef OBZ_TEXTURE_H
#define OBZ_TEXTURE_H

#include "SDL/Mesh/3dshapes.h"
#include <SDL2/SDL_image.h>

#define OBZ_MAX_MESH_TEXTURES 8

typedef struct {
    unsigned int id;
    int width;
    int height;
    unsigned char *pixels; // RGBA pixel data
} OBZ_Texture;

typedef struct {
    Mesh3D *mesh;                  // Mesh associated
    OBZ_Texture *textures[OBZ_MAX_MESH_TEXTURES];
} OBZ_MeshTextures;

OBZ_Texture *obz_tex_load_png(const char *path);
OBZ_MeshTextures *obz_tex_create_for_mesh(Mesh3D *mesh);

void obz_tex_free(OBZ_Texture *tex);
void obz_tex_bind_to_mesh(OBZ_MeshTextures *mt, OBZ_Texture *tex, int slot);
void obz_tex_unbind_from_mesh(OBZ_MeshTextures *mt, int slot);


#endif
