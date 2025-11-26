#ifndef OBZ_RENDER_H
#define OBZ_RENDER_H

#include "SDL/Mesh/3dshapes.h"
#include "SDL/core.h"
#include "SDL/shapes.h"

// this file is a goddamn misnomer; only generates edges there is no rasterization or textures being created yet.

typedef struct {
    float diffuse[3];
    float specular[3];
    float shininess;
} OBZ_Material;

/* -------------- High-performance version with clipping -------------- */
inline static void draw_mesh_wire_camera_fast(OBZ_Context *ctx, Vec3 pos, Mesh3D *m,
                                              float pitch, float yaw, float roll,
                                              int sw, int sh,
                                              OBZ_Color c,
                                              OBZ_Camera cam)
{
    // Stack allocation for small meshes, heap for large ones
    const int use_heap = m->nverts > 1024;
    
    int *px = use_heap ? malloc(sizeof(int) * m->nverts) : alloca(sizeof(int) * m->nverts);
    int *py = use_heap ? malloc(sizeof(int) * m->nverts) : alloca(sizeof(int) * m->nverts);
    
    float cx = cosf(yaw), sx = sinf(yaw);
    float cy = cosf(pitch), sy = sinf(pitch);
    float cz = cosf(roll), sz = sinf(roll);
    
    // Transform and project
    for (int i = 0; i < m->nverts; i++) {
        Vec3 v = m->verts[i];
        
        // Combined rotation
        float x1 = v.x * cx - v.z * sx;
        float z1 = v.x * sx + v.z * cx;
        float y1 = v.y * cy - z1 * sy;
        float z2 = v.y * sy + z1 * cy;
        float x2 = x1 * cz - y1 * sz;
        float y2 = x1 * sz + y1 * cz;
        
        Vec3 world = { x2 + pos.x, y2 + pos.y, z2 + pos.z };
        project_camera(world, cam, &px[i], &py[i], sw, sh);
    }
    
    // Draw with screen bounds checking
    for (int i = 0; i < m->nfaces; i++) {
        int *f = m->faces[i];
        
        // Simple screen bounds check before drawing
        for (int j = 0; j < 4; j++) {
            int next = (j + 1) % 4;
            int v1 = f[j], v2 = f[next];
            
            if (px[v1] >= 0 && py[v1] >= 0 && px[v1] < sw && py[v1] < sh &&
                px[v2] >= 0 && py[v2] >= 0 && px[v2] < sw && py[v2] < sh) {
                obz_draw_line(ctx, 
                    (OBZ_Point){px[v1], py[v1]}, 
                    (OBZ_Point){px[v2], py[v2]}, c);
            }
        }
    }
    
    if (use_heap) {
        free(px);
        free(py);
    }
}

#endif
