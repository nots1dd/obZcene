#ifndef OBZ_COLORS_H
#define OBZ_COLORS_H

#include "obz_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /* ---------- Colors ---------- */

  // channel is ui8
  //
  // THING TO NOTE:
  //
  // The color channels are stored in RGBA order in the struct,
  // but when packed into a 32-bit integer for rendering, they are
  // arranged in AGBR order.
  //
  // {WHY}
  // CONCEPTUALLY, colors are often represented in RGBA order.
  // but SDL seems to be working only with ABGR idk y
  typedef struct
  {
    OBZ_channel r, g, b, a;
  } OBZ_Color;

  /* Common opaque utf8-ish names */
  static const OBZ_Color COLOR_TRANSPARENT = {0, 0, 0, 0};
  static const OBZ_Color COLOR_WHITE       = {255, 255, 255, 255};
  static const OBZ_Color COLOR_BLACK       = {0, 0, 0, 255};
  static const OBZ_Color COLOR_RED         = {255, 0, 0, 255};
  static const OBZ_Color COLOR_GREEN       = {0, 255, 0, 255};
  static const OBZ_Color COLOR_BLUE        = {0, 0, 255, 255};
  static const OBZ_Color COLOR_YELLOW      = {255, 255, 0, 255};
  static const OBZ_Color COLOR_CYAN        = {0, 255, 255, 255};
  static const OBZ_Color COLOR_MAGENTA     = {255, 0, 255, 255};
  static const OBZ_Color COLOR_GRAY        = {128, 128, 128, 255};
  static const OBZ_Color COLOR_DARK_GRAY   = {64, 64, 64, 255};
  static const OBZ_Color COLOR_LIGHT_GRAY  = {192, 192, 192, 255};
  static const OBZ_Color COLOR_ORANGE      = {255, 165, 0, 255};
  static const OBZ_Color COLOR_PURPLE      = {128, 0, 128, 255};
  static const OBZ_Color COLOR_BROWN       = {165, 42, 42, 255};

  static const OBZ_Color COLOR_NAVY       = {0, 0, 128, 255};
  static const OBZ_Color COLOR_TEAL       = {0, 128, 128, 255};
  static const OBZ_Color COLOR_OLIVE      = {128, 128, 0, 255};
  static const OBZ_Color COLOR_MAROON     = {128, 0, 0, 255};
  static const OBZ_Color COLOR_LIME       = {191, 255, 0, 255};
  static const OBZ_Color COLOR_SILVER     = {192, 192, 192, 255};
  static const OBZ_Color COLOR_GOLD       = {255, 215, 0, 255};
  static const OBZ_Color COLOR_SKY_BLUE   = {135, 206, 235, 255};
  static const OBZ_Color COLOR_STEEL_BLUE = {70, 130, 180, 255};
  static const OBZ_Color COLOR_ROYAL_BLUE = {65, 105, 225, 255};

  static const OBZ_Color COLOR_PINK     = {255, 192, 203, 255};
  static const OBZ_Color COLOR_HOT_PINK = {255, 105, 180, 255};
  static const OBZ_Color COLOR_SALMON   = {250, 128, 114, 255};
  static const OBZ_Color COLOR_CORAL    = {255, 127, 80, 255};

  static const OBZ_Color COLOR_VIOLET = {238, 130, 238, 255};
  static const OBZ_Color COLOR_INDIGO = {75, 0, 130, 255};

  static const OBZ_Color COLOR_FOREST_GREEN = {34, 139, 34, 255};
  static const OBZ_Color COLOR_LAWN_GREEN   = {124, 252, 0, 255};
  static const OBZ_Color COLOR_SEA_GREEN    = {46, 139, 87, 255};
  static const OBZ_Color COLOR_AQUA         = {0, 255, 255, 255};

  static const OBZ_Color COLOR_BEIGE = {245, 245, 220, 255};
  static const OBZ_Color COLOR_TAN   = {210, 180, 140, 255};
  static const OBZ_Color COLOR_KHAKI = {195, 176, 145, 255};

  static const OBZ_Color COLOR_CHOCOLATE  = {210, 105, 30, 255};
  static const OBZ_Color COLOR_DARK_RED   = {139, 0, 0, 255};
  static const OBZ_Color COLOR_DARK_GREEN = {0, 100, 0, 255};
  static const OBZ_Color COLOR_DARK_BLUE  = {0, 0, 139, 255};

  static const OBZ_Color COLOR_MINT     = {189, 252, 201, 255};
  static const OBZ_Color COLOR_LAVENDER = {230, 230, 250, 255};
  static const OBZ_Color COLOR_PEACH    = {255, 218, 185, 255};

  static const OBZ_Color COLOR_NEON_GREEN = {57, 255, 20, 255};
  static const OBZ_Color COLOR_NEON_BLUE  = {77, 77, 255, 255};
  static const OBZ_Color COLOR_NEON_PINK  = {255, 20, 147, 255};

  static const OBZ_Color COLOR_SMOKE_GRAY = {105, 105, 105, 255};
  static const OBZ_Color COLOR_CHARCOAL   = {54, 69, 79, 255};
  static const OBZ_Color COLOR_JET_BLACK  = {10, 10, 10, 255};

#ifdef __cplusplus
}
#endif
#endif /* COLORS_H */
