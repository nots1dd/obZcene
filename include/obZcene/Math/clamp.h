#ifndef OBZ_MATH_CLAMP_H
#define OBZ_MATH_CLAMP_H

#include "Math/floorceil.h"
#include "obz_types.h"

#define OBZ_CLAMP_MACRO_DECL(type, name)                             \
  OBZ_API_INLINE static type obz_clamp##name(type v, type lo, type hi) \
  {                                                                  \
    if (v < lo)                                                      \
      return lo;                                                     \
    if (v > hi)                                                      \
      return hi;                                                     \
    return v;                                                        \
  }

OBZ_CLAMP_MACRO_DECL(i8, i8)
OBZ_CLAMP_MACRO_DECL(i16, i16)
OBZ_CLAMP_MACRO_DECL(i32, i32)
OBZ_CLAMP_MACRO_DECL(i64, i64)

OBZ_CLAMP_MACRO_DECL(int, i)

OBZ_CLAMP_MACRO_DECL(ui8, ui8)
OBZ_CLAMP_MACRO_DECL(ui16, ui16)
OBZ_CLAMP_MACRO_DECL(ui32, ui32)
OBZ_CLAMP_MACRO_DECL(ui64, ui64)

OBZ_CLAMP_MACRO_DECL(uint, ui)

OBZ_CLAMP_MACRO_DECL(float, f)

OBZ_CLAMP_MACRO_DECL(double, d)

OBZ_API_INLINE static float obz_clampf01(float x) { return obz_clampf(x, 0.0f, 1.0f); }

OBZ_API_INLINE static double obz_clampd01(double x) { return obz_clampd(x, 0.0, 1.0); }

OBZ_API_INLINE static int obz_clamp_floor_to_int(float v, int lo, int hi)
{
  return obz_clampi((int)obz_floorf(v), lo, hi);
}

OBZ_API_INLINE static int obz_clamp_ceil_to_int(float v, int lo, int hi)
{
  return obz_clampi((int)obz_ceilf(v), lo, hi);
}

OBZ_API_INLINE static int obz_clampf_to_int(float x, int max_target)
{
  x = obz_clampf01(x); // first clamp to [0,1]
  return (int)(x * max_target);
}

#endif
