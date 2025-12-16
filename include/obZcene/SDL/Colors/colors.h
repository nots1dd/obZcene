#ifndef OBZ_COLORS_H
#define OBZ_COLORS_H

#include "Math/clamp.h"
#include "obz_macros.h"
#include "obz_types.h"
#include <stdint.h>

OBZ_BEGIN_CPP_DECLS

typedef struct
{
  OBZ_channel r, g, b, a;
} OBZ_Color;

#define OBZ_PACK_COLOR_COMPTIME(c) (((c).a << 24) | ((c).b << 16) | ((c).g << 8) | (c).r)

// !!AGBR scheme!!
OBZ_INTERNAL_INLINE static OBZ_pixel __OBZ_pack_color_channels(OBZ_channel r, OBZ_channel g,
                                                               OBZ_channel b, OBZ_channel a)
{
  return (a << 24) | (b << 16) | (g << 8) | r;
}

OBZ_INTERNAL_INLINE static OBZ_pixel __OBZ_pack_color(OBZ_Color c)
{
  return __OBZ_pack_color_channels(c.r, c.g, c.b, c.a);
}

OBZ_INTERNAL_INLINE static OBZ_channel __OBZ_clampf_to_channel(float x)
{
  return (OBZ_channel)obz_clampf_to_int(x, 255);
}

// X-Macro pattern: NAME, r, g, b, a
#define OBZ_COLOR_LIST                    \
  X(COLOR_TRANSPARENT, 0, 0, 0, 0)        \
  X(COLOR_WHITE, 255, 255, 255, 255)      \
  X(COLOR_BLACK, 0, 0, 0, 255)            \
  X(COLOR_RED, 255, 0, 0, 255)            \
  X(COLOR_GREEN, 0, 255, 0, 255)          \
  X(COLOR_BLUE, 0, 0, 255, 255)           \
  X(COLOR_YELLOW, 255, 255, 0, 255)       \
  X(COLOR_CYAN, 0, 255, 255, 255)         \
  X(COLOR_MAGENTA, 255, 0, 255, 255)      \
  X(COLOR_GRAY, 128, 128, 128, 255)       \
  X(COLOR_DARK_GRAY, 64, 64, 64, 255)     \
  X(COLOR_LIGHT_GRAY, 192, 192, 192, 255) \
  X(COLOR_ORANGE, 255, 165, 0, 255)       \
  X(COLOR_PURPLE, 128, 0, 128, 255)       \
  X(COLOR_BROWN, 165, 42, 42, 255)        \
  X(COLOR_NAVY, 0, 0, 128, 255)           \
  X(COLOR_TEAL, 0, 128, 128, 255)         \
  X(COLOR_OLIVE, 128, 128, 0, 255)        \
  X(COLOR_MAROON, 128, 0, 0, 255)         \
  X(COLOR_LIME, 191, 255, 0, 255)         \
  X(COLOR_SILVER, 192, 192, 192, 255)     \
  X(COLOR_GOLD, 255, 215, 0, 255)         \
  X(COLOR_SKY_BLUE, 135, 206, 235, 255)   \
  X(COLOR_STEEL_BLUE, 70, 130, 180, 255)  \
  X(COLOR_ROYAL_BLUE, 65, 105, 225, 255)  \
  X(COLOR_PINK, 255, 192, 203, 255)       \
  X(COLOR_HOT_PINK, 255, 105, 180, 255)   \
  X(COLOR_SALMON, 250, 128, 114, 255)     \
  X(COLOR_CORAL, 255, 127, 80, 255)       \
  X(COLOR_VIOLET, 238, 130, 238, 255)     \
  X(COLOR_INDIGO, 75, 0, 130, 255)        \
  X(COLOR_FOREST_GREEN, 34, 139, 34, 255) \
  X(COLOR_LAWN_GREEN, 124, 252, 0, 255)   \
  X(COLOR_SEA_GREEN, 46, 139, 87, 255)    \
  X(COLOR_AQUA, 0, 255, 255, 255)         \
  X(COLOR_BEIGE, 245, 245, 220, 255)      \
  X(COLOR_TAN, 210, 180, 140, 255)        \
  X(COLOR_KHAKI, 195, 176, 145, 255)      \
  X(COLOR_CHOCOLATE, 210, 105, 30, 255)   \
  X(COLOR_DARK_RED, 139, 0, 0, 255)       \
  X(COLOR_DARK_GREEN, 0, 100, 0, 255)     \
  X(COLOR_DARK_BLUE, 0, 0, 139, 255)      \
  X(COLOR_MINT, 189, 252, 201, 255)       \
  X(COLOR_LAVENDER, 230, 230, 250, 255)   \
  X(COLOR_PEACH, 255, 218, 185, 255)      \
  X(COLOR_NEON_GREEN, 57, 255, 20, 255)   \
  X(COLOR_NEON_BLUE, 77, 77, 255, 255)    \
  X(COLOR_NEON_PINK, 255, 20, 147, 255)   \
  X(COLOR_SMOKE_GRAY, 105, 105, 105, 255) \
  X(COLOR_CHARCOAL, 54, 69, 79, 255)      \
  X(COLOR_JET_BLACK, 10, 10, 10, 255)

// ----------------- OBZ_Color constants -----------------
#define X(name, r, g, b, a) static const OBZ_Color name = {r, g, b, a};
OBZ_COLOR_LIST
#undef X

// ----------------- OBZ_pixel constants -----------------
#define X(name, r, g, b, a) static const OBZ_pixel name##_PIXELS = OBZ_PACK_COLOR_COMPTIME(name);
OBZ_COLOR_LIST
#undef X

OBZ_END_CPP_DECLS

#endif /* OBZ_COLORS_H */
