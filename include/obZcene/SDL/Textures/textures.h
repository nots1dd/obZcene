#ifndef OBZ_SDL_TEXTURE_H
#define OBZ_SDL_TEXTURE_H

#include "SDL/Mesh/mesh.h"
#include "types.h"
#include <SDL2/SDL_image.h>

// can change this as time goes on but 16 is enough for now
#define OBZ_MAX_MESH_TEXTURES 16
#define OBZ_MAX_MESH_FACES    128

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct
  {
    unsigned int id;
    int          width;
    int          height;
    OBZ_channel* pixels; // RGBA pixel data
  } OBZ_Texture;

  typedef struct
  {
    OBZ_Mesh3D*  mesh; // Mesh associated
    OBZ_Texture* textures[OBZ_MAX_MESH_TEXTURES];
    int          face_tex_index[OBZ_MAX_MESH_FACES]; // Maps each face to a texture slot
  } OBZ_MeshTextures;

  OBZ_API OBZ_Texture*      obz_tex_load_png(const char* path);
  OBZ_API OBZ_MeshTextures* obz_tex_create_for_mesh(OBZ_Mesh3D* mesh);

  OBZ_API void         obz_tex_free(OBZ_Texture* tex);
  OBZ_API void         obz_tex_bind_to_mesh(OBZ_MeshTextures* mt, OBZ_Texture* tex, int slot);
  OBZ_API void         obz_tex_unbind_from_mesh(OBZ_MeshTextures* mt, int slot);
  OBZ_API void         obz_tex_set_face_texture(OBZ_MeshTextures* mt, int face_index, int tex_slot);
  OBZ_API OBZ_Texture* obz_tex_get_face_texture(OBZ_MeshTextures* mt, int face_index);

#ifdef __cplusplus
}
#endif

#endif /* OBZ_SDL_TEXTURE_H */
