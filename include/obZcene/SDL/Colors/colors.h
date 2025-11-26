#ifndef OBZ_COLORS_H
#define OBZ_COLORS_H

#include "types.h"
#include <stdint.h>

/* ---------- Colors ---------- */

// pixel -> ui8 -> uint8_t -> unsigned char (coz char is just int8_t)
//
// so yea typedef galore here
typedef struct
{
  pixel r, g, b, a;
} OBZ_Color;

#ifdef __cplusplus
extern "C"
{
#endif

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

  static const OBZ_Color COLOR_NAVY   = {0, 0, 128, 255};
  static const OBZ_Color COLOR_TEAL   = {0, 128, 128, 255};
  static const OBZ_Color COLOR_OLIVE  = {128, 128, 0, 255};
  static const OBZ_Color COLOR_MAROON = {128, 0, 0, 255};
  static const OBZ_Color COLOR_LIME   = {191, 255, 0, 255};

#ifdef __cplusplus
}
#endif
#endif /* COLORS_H */
