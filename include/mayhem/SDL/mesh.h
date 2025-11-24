#ifndef MHM_SDL_MESH_H
#define MHM_SDL_MESH_H

#include "SDL/shapes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Mesh3D *items;
    int count;
} MeshCache;

static MeshCache g_meshes = {0};

inline static int mesh_cache_add(const Mesh3D *m) {
    g_meshes.items = realloc(g_meshes.items, sizeof(Mesh3D) * (g_meshes.count + 1));
    g_meshes.items[g_meshes.count] = *m;
    return g_meshes.count++;
}

inline static void free_mesh(Mesh3D *m) {
    free(m->verts);
    free(m->faces);
    m->verts = NULL;
    m->faces = NULL;
    m->nverts = m->nfaces = 0;
}

inline static void mesh_cache_free_all(void) {
    for (int i = 0; i < g_meshes.count; i++)
        free_mesh(&g_meshes.items[i]);
    free(g_meshes.items);
    g_meshes.items = NULL;
    g_meshes.count = 0;
}

inline static Mesh3D make_quad(float w, float h) {
    Mesh3D m;

    m.nverts = 4;
    m.verts = malloc(sizeof(Vec3) * 4);

    float hw = w * 0.5f;
    float hh = h * 0.5f;

    m.verts[0] = (Vec3){-hw, -hh, 0};
    m.verts[1] = (Vec3){ hw, -hh, 0};
    m.verts[2] = (Vec3){ hw,  hh, 0};
    m.verts[3] = (Vec3){-hw,  hh, 0};

    m.nfaces = 2;
    m.faces  = alloc_faces(m.nfaces);
    
    static int F[2][4] = {
        {0,1,2,3},
        {0,2,3,3}
    };

    memcpy(m.faces, F, sizeof(F));
    
    return m;
}

/* ---------------- Cube generator ---------------- */
inline static Mesh3D make_cube(float size) {
    Mesh3D m;
    m.nverts = 8;
    m.nfaces = 6;

    m.verts  = malloc(sizeof(Vec3) * m.nverts);
    m.faces  = alloc_faces(m.nfaces);

    float h = size/2;

    m.verts[0]=(Vec3){-h,-h,-h}; m.verts[1]=(Vec3){ h,-h,-h};
    m.verts[2]=(Vec3){ h, h,-h}; m.verts[3]=(Vec3){-h, h,-h};
    m.verts[4]=(Vec3){-h,-h, h}; m.verts[5]=(Vec3){ h,-h, h};
    m.verts[6]=(Vec3){ h, h, h}; m.verts[7]=(Vec3){-h, h, h};

    static int F[6][4] = {
        {0,1,2,3},
        {4,5,6,7},
        {0,1,5,4},
        {2,3,7,6},
        {0,3,7,4},
        {1,2,6,5}
    };
    memcpy(m.faces, F, sizeof(F));
    return m;
}

inline static Mesh3D make_pyramid(float size, float height) {
    Mesh3D m;
    m.nverts = 5;
    m.nfaces = 5;

    m.verts = malloc(sizeof(Vec3) * m.nverts);
    m.faces = alloc_faces(m.nfaces);

    float h = size/2;

    m.verts[0]=(Vec3){-h,0,-h};
    m.verts[1]=(Vec3){ h,0,-h};
    m.verts[2]=(Vec3){ h,0, h};
    m.verts[3]=(Vec3){-h,0, h};
    m.verts[4]=(Vec3){0,height,0};

    static int F[5][4] = {
        {0,1,2,3},
        {0,1,4,4},
        {1,2,4,4},
        {2,3,4,4},
        {3,0,4,4}
    };
    memcpy(m.faces, F, sizeof(F));
    return m;
}

inline static Mesh3D make_sphere(float r, int seg) {
    int nverts = (seg+1)*(seg+1);
    int nfaces = seg*seg;

    Mesh3D m;
    m.nverts = nverts;
    m.nfaces = nfaces;
    m.verts = malloc(sizeof(Vec3) * nverts);
    m.faces = alloc_faces(nfaces);

    int idx = 0;
    for(int i=0;i<=seg;i++){
        float t = (float)i / seg * M_PI;
        for(int j=0;j<=seg;j++){
            float p = (float)j / seg * 2*M_PI;
            m.verts[idx++] = (Vec3){
                r * sinf(t)*cosf(p),
                r * cosf(t),
                r * sinf(t)*sinf(p)
            };
        }
    }

    int f = 0;
    for(int i=0;i<seg;i++){
        for(int j=0;j<seg;j++){
            int v0 = i*(seg+1) + j;
            int v1 = v0 + 1;
            int v2 = v0 + (seg+1) + 1;
            int v3 = v0 + (seg+1);
            m.faces[f][0]=v0;
            m.faces[f][1]=v1;
            m.faces[f][2]=v2;
            m.faces[f][3]=v3;
            f++;
        }
    }

    return m;
}

inline static Mesh3D make_wire_plane(float w, float h, int tilesX, int tilesY) {
    Mesh3D m = {0};

    if (tilesX <= 0) tilesX = 1;
    if (tilesY <= 0) tilesY = 1;

    int vertsX = tilesX + 1;
    int vertsY = tilesY + 1;

    /* verts */
    m.nverts = vertsX * vertsY;
    m.verts = malloc(sizeof(Vec3) * m.nverts);
    if (!m.verts) { m.nverts = 0; return m; }

    float dx = w / tilesX;
    float dy = h / tilesY;
    int idx = 0;
    for (int iy = 0; iy < vertsY; iy++) {
        float y = -h * 0.5f + iy * dy;
        for (int ix = 0; ix < vertsX; ix++) {
            float x = -w * 0.5f + ix * dx;
            m.verts[idx++] = (Vec3){ x, y, 0.0f };
        }
    }

    /* faces (quads) */
    m.nfaces = tilesX * tilesY;
    m.faces = malloc(sizeof(int[4]) * m.nfaces);
    if (!m.faces) {
        free(m.verts);
        m.verts = NULL;
        m.nverts = 0;
        m.nfaces = 0;
        return m;
    }

    int f = 0;
    for (int iy = 0; iy < tilesY; iy++) {
        for (int ix = 0; ix < tilesX; ix++) {
            int v0 = iy * vertsX + ix;
            int v1 = v0 + 1;
            int v2 = v0 + vertsX + 1;
            int v3 = v0 + vertsX;
            m.faces[f][0] = v0;
            m.faces[f][1] = v1;
            m.faces[f][2] = v2;
            m.faces[f][3] = v3;
            f++;
        }
    }

    return m;
}


#endif
