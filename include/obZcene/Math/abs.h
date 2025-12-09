#ifndef OBZ_MATH_ABS_H
#define OBZ_MATH_ABS_H

#include "obz_macros.h"
#include "obz_types.h"
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

OBZ_BEGIN_CPP_DECLS

/* ------------------------------------------------------------------------- */
/* FLOAT ABS (bit clear, memcpy-safe) */
/* ------------------------------------------------------------------------- */

OBZ_API_INLINE static float obz_fabsf(float x)
{
  uint32_t u;
  memcpy(&u, &x, sizeof u);
  u &= 0x7FFFFFFFu;
  memcpy(&x, &u, sizeof x);
  return x;
}

OBZ_API_INLINE static double obz_fabs(double x)
{
  ui64 u;
  memcpy(&u, &x, sizeof u);
  u &= 0x7FFFFFFFFFFFFFFFull;
  memcpy(&x, &u, sizeof x);
  return x;
}

// long double is not support due to platform differences in representation
//
// some platforms use 80-bit extended precision, others use 128-bit quadruple precision
// so we fallback to standard library function for portability
OBZ_API_INLINE static ld obz_fabsl(ld x) { return fabsl(x); }

/* ------------------------------------------------------------------------- */
/* INTEGER ABS (branchless, unsigned return — safe for INT_MIN) */
/* ------------------------------------------------------------------------- */

OBZ_API_INLINE static ui8 obz_abs_i8(i8 x)
{
  i8 s = x >> 7;
  return (ui8)((x ^ s) - s);
}

OBZ_API_INLINE static ui16 obz_abs_i16(i16 x)
{
  i16 s = x >> 15;
  return (ui16)((x ^ s) - s);
}

OBZ_API_INLINE static ui32 obz_abs_i32(i32 x)
{
  i32 s = x >> 31;
  return (ui32)((x ^ s) - s);
}

OBZ_API_INLINE static ui64 obz_abs_i64(i64 x)
{
  i64 s = x >> 63;
  return (ui64)((x ^ s) - s);
}

/* ------ saturating version (signed return, clamps INT_MIN) ------ */

OBZ_API_INLINE static i32 obz_saturating_abs32(i32 x)
{
  ui32 ux = obz_abs_i32(x);
  return (ux > INT32_MAX) ? INT32_MAX : (i32)ux;
}

OBZ_API_INLINE static i64 obz_saturating_abs64(i64 x)
{
  ui64 ux = obz_abs_i64(x);
  return (ux > INT64_MAX) ? INT64_MAX : (i64)ux;
}

#define OBZ_GEN_UNSIGNED_ID(T, name) \
  OBZ_API_INLINE static T name(T v) { return v; }

OBZ_GEN_UNSIGNED_ID(ui8, obz_identity_ui8)
OBZ_GEN_UNSIGNED_ID(ui16, obz_identity_ui16)
OBZ_GEN_UNSIGNED_ID(ui32, obz_identity_ui32)
OBZ_GEN_UNSIGNED_ID(ui64, obz_identity_ui64)

/* ------------------------------------------------------------------------- */
/* GENERIC ABS SELECTOR */
/* ------------------------------------------------------------------------- */

#define obz_abs(x)           \
  _Generic((x),              \
    float: obz_fabsf,        \
    double: obz_fabs,        \
    ld: obz_fabsl,           \
                             \
    i8: obz_abs_i8,          \
    i16: obz_abs_i16,        \
    i32: obz_abs_i32,        \
    i64: obz_abs_i64,        \
                             \
    ui8: obz_identity_ui8,   \
    ui16: obz_identity_ui16, \
    ui32: obz_identity_ui32, \
    ui64: obz_identity_ui64)(x)

OBZ_END_CPP_DECLS
#endif
