#ifndef OBZ_MATH_CLAMP_H
#define OBZ_MATH_CLAMP_H

#include "Math/floorceil.h"
#include "obz_types.h"

inline static int obz_clampi(int v, int lo, int hi)
{
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static inline float obz_clampf(float x, float min_val, float max_val)
{
  if (x < min_val)
    return min_val;
  if (x > max_val)
    return max_val;
  return x;
}

static inline float obz_clampf01(float x) { return obz_clampf(x, 0.0f, 1.0f); }

static inline double obz_clampd(double x, double min_val, double max_val)
{
  if (x < min_val)
    return min_val;
  if (x > max_val)
    return max_val;
  return x;
}

inline static int obz_clamp_floor_to_int(float v, int lo, int hi)
{
  return obz_clampi((int)obz_floorf(v), lo, hi);
}

inline static int obz_clamp_ceil_to_int(float v, int lo, int hi)
{
  return obz_clampi((int)obz_ceilf(v), lo, hi);
}

static inline int obz_clampf_to_int(float x, int max_target)
{
  x = obz_clampf01(x); // first clamp to [0,1]
  return (int)(x * max_target);
}

static inline OBZ_channel __OBZ_clampf_to_channel(float x)
{
  return (OBZ_channel)obz_clampf_to_int(x, 255);
}

#endif
