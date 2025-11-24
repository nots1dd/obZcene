#include "SDL/texture.h"

static unsigned int g_tex_id_counter = 1;

MHMAPI_Texture *mhmapi_tex_load_png(const char *path)
{
    SDL_Surface *surf = IMG_Load(path);
    if (!surf) {
        printf("Failed to load PNG surface '%s': %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);

    if (!rgba) {
        printf("ConvertSurfaceFormat failed: %s\n", SDL_GetError());
        return NULL;
    }

    MHMAPI_Texture *t = malloc(sizeof(MHMAPI_Texture));
    if (!t) {
        SDL_FreeSurface(rgba);
        return NULL;
    }

    t->width  = rgba->w;
    t->height = rgba->h;
    t->id     = g_tex_id_counter++;

    size_t size = (size_t)t->width * t->height * 4;
    t->pixels = malloc(size);

    if (!t->pixels) {
        free(t);
        SDL_FreeSurface(rgba);
        return NULL;
    }

    memcpy(t->pixels, rgba->pixels, size);
    SDL_FreeSurface(rgba);

    return t;
}

void mhmapi_tex_free(MHMAPI_Texture *tex)
{
    if (!tex) return;
    free(tex->pixels);
    free(tex);
}

MHMAPI_MeshTextures *mhmapi_tex_create_for_mesh(Mesh3D *mesh)
{
    MHMAPI_MeshTextures *mt = malloc(sizeof(MHMAPI_MeshTextures));
    if (!mt) return NULL;

    mt->mesh = mesh;
    memset(mt->textures, 0, sizeof(mt->textures));
    return mt;
}

void mhmapi_tex_bind_to_mesh(MHMAPI_MeshTextures *mt, MHMAPI_Texture *tex, int slot)
{
    if (!mt || !tex) return;
    if (slot < 0 || slot >= MHMAPI_MAX_MESH_TEXTURES) return;
    mt->textures[slot] = tex;
}

void mhmapi_tex_unbind_from_mesh(MHMAPI_MeshTextures *mt, int slot)
{
    if (!mt) return;
    if (slot < 0 || slot >= MHMAPI_MAX_MESH_TEXTURES) return;
    mt->textures[slot] = NULL;
}
