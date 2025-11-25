#ifndef MHMAPI_TEXTURE_H
#define MHMAPI_TEXTURE_H

#include "SDL/Mesh/3dshapes.h"
#include <SDL2/SDL_image.h>

#define MHMAPI_MAX_MESH_TEXTURES 8

typedef struct {
    unsigned int id;
    int width;
    int height;
    unsigned char *pixels; // RGBA pixel data
} MHMAPI_Texture;

typedef struct {
    Mesh3D *mesh;                  // Mesh associated
    MHMAPI_Texture *textures[MHMAPI_MAX_MESH_TEXTURES];
} MHMAPI_MeshTextures;

MHMAPI_Texture *mhmapi_tex_load_png(const char *path);
MHMAPI_MeshTextures *mhmapi_tex_create_for_mesh(Mesh3D *mesh);

void mhmapi_tex_free(MHMAPI_Texture *tex);
void mhmapi_tex_bind_to_mesh(MHMAPI_MeshTextures *mt, MHMAPI_Texture *tex, int slot);
void mhmapi_tex_unbind_from_mesh(MHMAPI_MeshTextures *mt, int slot);


#endif
