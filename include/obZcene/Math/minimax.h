#ifndef OBZ_FAST_MINMAX_H
#define OBZ_FAST_MINMAX_H

#include "obz_macros.h"
#include "obz_types.h"

OBZ_BEGIN_CPP_DECLS

#define OBZ_MINIMAX_MACRO_DECL(type, name)                                           \
  OBZ_API_INLINE static type obz_min##name(type a, type b) { return (a < b) ? a : b; } \
  OBZ_API_INLINE static type obz_max##name(type a, type b) { return (a > b) ? a : b; }

OBZ_MINIMAX_MACRO_DECL(i8, i8)
OBZ_MINIMAX_MACRO_DECL(i16, i16)
OBZ_MINIMAX_MACRO_DECL(i32, i32)
OBZ_MINIMAX_MACRO_DECL(i64, i64)

OBZ_MINIMAX_MACRO_DECL(int, i)

OBZ_MINIMAX_MACRO_DECL(ui8, ui8)
OBZ_MINIMAX_MACRO_DECL(ui16, ui16)
OBZ_MINIMAX_MACRO_DECL(ui32, ui32)
OBZ_MINIMAX_MACRO_DECL(ui64, ui64)

OBZ_MINIMAX_MACRO_DECL(uint, ui)

OBZ_MINIMAX_MACRO_DECL(float, f)

OBZ_MINIMAX_MACRO_DECL(double, d)

OBZ_END_CPP_DECLS

#endif /* OBZ_FAST_MINMAX_H */
