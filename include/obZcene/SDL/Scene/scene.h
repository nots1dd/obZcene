#ifndef OBZ_SDL_SCENE_H
#define OBZ_SDL_SCENE_H

#include "SDL/Colors/colors.h"
#include "SDL/Mesh/mesh.h"
#include "SDL/Textures/textures.h"
#include <stdlib.h>

typedef Vec3 Rot3;

typedef struct {
    Mesh3D **meshes;           /* pointers to meshes (do not copy Mesh3D by value) */
    Vec3   *positions;
    Rot3   *rotations;
    OBZ_Color *colors;
    OBZ_MeshTextures *textures;
    ui8 *owned;            /* 1 => this scene is owner and must free the mesh */
    int count;
    int capacity;
    Rot3 *rotSpeeds;

    float time;
} Scene;


static Scene g_scene = {0};

inline static void scene_update(float dt) {
    for (int i = 0; i < g_scene.count; i++) {
        Vec3 s = g_scene.rotSpeeds[i];
        if (s.x != 0 || s.y != 0 || s.z != 0) {
            g_scene.rotations[i].x += s.x * dt;
            g_scene.rotations[i].y += s.y * dt;
            g_scene.rotations[i].z += s.z * dt;
        }
    }

    g_scene.time += dt;
}

inline static Vec3 deg_to_rad3(Rot3 d) {
    const float k = (float)M_PI / 180.0f;
    return (Rot3){ d.x*k, d.y*k, d.z*k };
}

/* ensure capacity (doubling) */
inline static void scene_ensure_capacity(int need) {
    if (g_scene.capacity >= need) return;
    int cap = g_scene.capacity ? g_scene.capacity : 8;
    while (cap < need) cap *= 2;
    g_scene.meshes    = realloc(g_scene.meshes,    sizeof(Mesh3D*) * cap);
    g_scene.positions = realloc(g_scene.positions, sizeof(Vec3)    * cap);
    g_scene.rotations = realloc(g_scene.rotations, sizeof(Rot3)    * cap);
    g_scene.rotSpeeds = realloc(g_scene.rotSpeeds, sizeof(Rot3) * cap);
    g_scene.colors    = realloc(g_scene.colors,    sizeof(OBZ_Color) * cap);
    g_scene.owned     = realloc(g_scene.owned,     sizeof(ui8) * cap);
    g_scene.textures  = realloc(g_scene.textures,  sizeof(OBZ_MeshTextures) * cap);
    g_scene.capacity  = cap;
}

/* Add a mesh pointer to the scene.
   If own == 1, scene_free_all() will call free_mesh() and free() the pointer.
   If own == 0, scene only references the mesh (do not free it in scene_free_all). */
inline static int scene_add_mesh_ptr(
    Mesh3D *m,
    Vec3 pos,
    Rot3 rot,
    OBZ_Color col,
    int own,
    Rot3 rotSpeed
) {
    if (!m) return -1;
    int id = g_scene.count;
    scene_ensure_capacity(g_scene.count + 1);

    g_scene.meshes[id]    = m;
    g_scene.positions[id] = pos;
    g_scene.rotations[id] = rot;
    g_scene.colors[id]    = col;
    g_scene.owned[id]     = (ui8)(own ? 1 : 0);
    g_scene.rotSpeeds[id] = rotSpeed;

    if (g_scene.textures) {
        g_scene.textures[id] = (OBZ_MeshTextures){0};
    }

    g_scene.count++;
    return id;
}

/* Free only meshes that scene owns, and free arrays */
inline static void scene_free_all(void) {
    for (int i = 0; i < g_scene.count; i++) {
        if (g_scene.owned && g_scene.owned[i]) {
            Mesh3D *m = g_scene.meshes[i];
            if (m) {
                free_mesh(m); /* frees verts/faces inside */
                free(m);      /* free the container struct */
            }
        }
    }

    free(g_scene.meshes);
    free(g_scene.positions);
    free(g_scene.rotations);
    free(g_scene.colors);
    free(g_scene.owned);
    free(g_scene.rotSpeeds);
    free(g_scene.textures);

    g_scene.meshes = NULL;
    g_scene.positions = NULL;
    g_scene.rotSpeeds = NULL;
    g_scene.rotations = NULL;
    g_scene.colors = NULL;
    g_scene.owned = NULL;
    g_scene.count = 0;
    g_scene.capacity = 0;
    g_scene.time = 0.0f;
}

inline static void scene_add_pyramid(
    Mesh3D *pyramid_ptr,
    Vec3 pos,
    Rot3 rot_deg,
    Rot3 speed_deg   /* NEW */
) {
    Rot3 rotRad   = deg_to_rad3(rot_deg);
    Vec3 speedRad = deg_to_rad3(speed_deg);

    scene_add_mesh_ptr(
        pyramid_ptr,
        pos,
        rotRad,
        COLOR_GREEN,
        0,
        speedRad
    );
}

inline static void scene_add_sphere(
    Mesh3D *sphere_ptr,
    float orbit_radius,
    float baseY,
    Rot3 rot_deg,
    Rot3 speed_deg   /* NEW */
) {
    static const OBZ_Color cols[3] = {
        COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW
    };

    Vec3 rotRad   = deg_to_rad3(rot_deg);
    Vec3 speedRad = deg_to_rad3(speed_deg);

    for (int i = 0; i < 3; i++) {
        scene_add_mesh_ptr(
            sphere_ptr,
            (Vec3){ orbit_radius, baseY, 0 },
            rotRad,
            cols[i],
            0,
            speedRad
        );
    }
}

inline static void scene_add_room(
    float W, float H, float D,
    int tiles,
    Rot3 base_rot, 
    Rot3 speed
) {
    float floorY = -H/2;
    float ceilY  =  H/2;

    Mesh3D *plane = NULL;

    // FLOOR
    plane = malloc(sizeof(Mesh3D));
    *plane = make_wire_plane(W, D, tiles, tiles);
    scene_add_mesh_ptr(
        plane,
        (Vec3){0, floorY, 0},
        (Rot3){-M_PI/2, 0, 0},
        COLOR_GRAY,
        1,
        speed
    );

    // CEILING
    plane = malloc(sizeof(Mesh3D));
    *plane = make_wire_plane(W, D, tiles, tiles);
    scene_add_mesh_ptr(
        plane,
        (Vec3){0, ceilY, 0},
        (Rot3){ M_PI/2, 0, 0},
        COLOR_GRAY,
        1,
        speed
    );

    // BACK WALL
    plane = malloc(sizeof(Mesh3D));
    *plane = make_wire_plane(W, H, tiles, tiles);
    scene_add_mesh_ptr(
        plane,
        (Vec3){0,0,-D/2},
        (Rot3){0,0,0},
        COLOR_GRAY,
        1,
        speed
    );

    // FRONT WALL
    plane = malloc(sizeof(Mesh3D));
    *plane = make_wire_plane(W, H, tiles, tiles);
    scene_add_mesh_ptr(
        plane,
        (Vec3){0,0, D/2},
        (Rot3){0,M_PI,0},
        COLOR_GRAY,
        1,
        speed
    );

    // LEFT WALL
    plane = malloc(sizeof(Mesh3D));
    *plane = make_wire_plane(D, H, tiles, tiles);
    scene_add_mesh_ptr(
        plane,
        (Vec3){-W/2,0,0},
        (Rot3){0,-M_PI/2,0},
        COLOR_GRAY,
        1,
        speed
    );

    // RIGHT WALL
    plane = malloc(sizeof(Mesh3D));
    *plane = make_wire_plane(D, H, tiles, tiles);
    scene_add_mesh_ptr(
        plane,
        (Vec3){ W/2,0,0},
        (Rot3){0,M_PI/2,0},
        COLOR_GRAY,
        1,
        speed
    );
}

#endif
