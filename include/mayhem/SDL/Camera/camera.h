#ifndef MHM_SDL_CAMERA_H
#define MHM_SDL_CAMERA_H

#include "Math/macros.h"
#include "Math/simd/x86/trig_pack.h"
#include <math.h>

typedef struct {
    Vec3 position;   // Camera position in world
    float pitch;     // Rotation up/down (radians)
    float yaw;       // Rotation left/right (radians)
    float roll;      // Rotation tilt (radians)
    float fov;       // Field of view in radians
    float aspect;    // Aspect ratio (width/height)
    float near;      // Near clipping plane
    float far;       // Far clipping plane
} Camera;

// Initialize camera with sensible defaults
inline static Camera camera_init(const Vec3 pos, const float W, const float H, const float fov_degrees) {
    Camera cam = {
        .position = pos,
        .pitch = 0.0f,
        .yaw = 0.0f,
        .roll = 0.0f,
        .fov = fov_degrees * M_PI / 180.0f,  // Convert to radians
        .aspect = W / H,
        .near = 0.1f,
        .far = 5*W
    };
    return cam;
}

// projection with FOV handling
inline static void project_camera(Vec3 world_pos, Camera cam,
                                  int *px, int *py,
                                  int sw, int sh)
{
    // Transform to camera space
    float dx = world_pos.x - cam.position.x;
    float dy = world_pos.y - cam.position.y;
    float dz = world_pos.z - cam.position.z;
    
    // Apply yaw rotation (around y axis)
    float cy = cosf(cam.yaw);
    float sy = sinf(cam.yaw);
    float x1 = dx * cy + dz * sy;
    float z1 = -dx * sy + dz * cy;
    
    // Apply pitch rotation (around x axis)
    float cp = cosf(cam.pitch);
    float sp = sinf(cam.pitch);
    float y1 = dy * cp - z1 * sp;
    float z2 = dy * sp + z1 * cp;
    
    // Apply roll rotation (optional, usually 0) (around z axis)
    float cr = cosf(cam.roll);
    float sr = sinf(cam.roll);
    float x2 = x1 * cr - y1 * sr;
    float y2 = x1 * sr + y1 * cr;
    
    // Clip to near plane
    if (z2 < cam.near) {
        *px = -1;
        *py = -1;
        return;
    }
    
    // Perspective projection with proper FOV
    float fov_scale = 1.0f / tanf(cam.fov * 0.5f);
    
    *px = (int)(sw * 0.5f + (x2 / z2) * fov_scale * sh * 0.5f);
    *py = (int)(sh * 0.5f - (y2 / z2) * fov_scale * sh * 0.5f);
}

#endif /* MHM_SDL_CAMERA_H */
