#ifndef MHM_MATH_MACROS_H
#define MHM_MATH_MACROS_H

#include <stdint.h>

/* ---- Types ---- */
typedef float mhm_f32;
typedef uint32_t mhm_u32;

// static const mhm_f32 PI = 3.14159265358979323846f;
// static const mhm_f32 TWO_PI = 6.28318530717958647692f;
// static const mhm_f32 PI_2 = 1.5707963267948966f;

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y; } Vec2;

#endif
