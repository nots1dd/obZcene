#ifndef GRAPHIKS_TYPES_H

#include <stdint.h>

typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

#endif
