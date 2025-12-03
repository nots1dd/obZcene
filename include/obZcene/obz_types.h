#ifndef OBZ_TYPES_H
#define OBZ_TYPES_H

#include "obz_macros.h"
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define auto __auto_type
#endif

OBZ_BEGIN_CPP_DECLS

/* ---------- Error codes ---------- */
typedef enum
{
  OBZ_OK = 0,
  OBZ_ERR_SDL,
  OBZ_ERR_FILE,
  OBZ_ERR_IMG,
  OBZ_ERR_ALLOC,
  OBZ_ERR_INVALID,
  OBZ_ERR_RUNTIME
} OBZ_Result;

/* ---------- Bool ---------- */
typedef enum
{
  OBZ_FALSE = 0,
  OBZ_TRUE  = 1
} OBZ_Bool;

typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned char uchar;

/* Floating point */
typedef float  f32;
typedef double f64;

/* Byte / pixel / bool variants */
typedef ui8  OBZ_byte;
typedef ui32 OBZ_pixel;
typedef ui8  OBZ_channel;

typedef ui8  OBZ_bool8;
typedef ui32 OBZ_bool32;

/* Compile-time sanity checks */
_Static_assert(sizeof(ui8) == 1, "u8 must be 1 byte");
_Static_assert(sizeof(ui16) == 2, "u16 must be 2 bytes");
_Static_assert(sizeof(ui32) == 4, "u32 must be 4 bytes");
_Static_assert(sizeof(ui64) == 8, "u64 must be 8 bytes");

_Static_assert(sizeof(i8) == 1, "i8 must be 1 byte");
_Static_assert(sizeof(i16) == 2, "i16 must be 2 bytes");
_Static_assert(sizeof(i32) == 4, "i32 must be 4 bytes");
_Static_assert(sizeof(i64) == 8, "i64 must be 8 bytes");

OBZ_END_CPP_DECLS

#endif /* OBZ_TYPES_H */
