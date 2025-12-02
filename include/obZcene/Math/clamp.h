#ifndef OBZ_MATH_CLAMP_H
#define OBZ_MATH_CLAMP_H

#include "obz_types.h"
#include <math.h>

inline static int clampi(int v, int lo, int hi)
{
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

// Clamp after floorf
inline static int clamp_floor_to_int(float v, int lo, int hi)
{
  return clampi((int)floorf(v), lo, hi);
}

// Clamp after ceilf
inline static int clamp_ceil_to_int(float v, int lo, int hi)
{
  return clampi((int)ceilf(v), lo, hi);
}

static inline float clampf(float x, float min_val, float max_val)
{
  if (x < min_val)
    return min_val;
  if (x > max_val)
    return max_val;
  return x;
}

static inline float clampf01(float x) { return clampf(x, 0.0f, 1.0f); }

static inline double clampd(double x, double min_val, double max_val)
{
  if (x < min_val)
    return min_val;
  if (x > max_val)
    return max_val;
  return x;
}

static inline int clampf_to_int(float x, int max_target)
{
  x = clampf01(x); // first clamp to [0,1]
  return (int)(x * max_target);
}

static inline OBZ_channel clampf_to_channel(float x) { return (OBZ_channel)clampf_to_int(x, 255); }

#endif
