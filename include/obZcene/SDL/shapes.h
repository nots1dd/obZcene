#ifndef OBZ_SHAPES_H
#define OBZ_SHAPES_H

#include <stdint.h>
#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Basic shapes ---------- */

typedef struct {
    int x, y;
} OBZ_Point;

typedef struct {
    int x, y, w, h;
} OBZ_Rect;

typedef struct {
    int cx, cy;
    int radius;
} OBZ_Circle;

/* ---------- Colors ---------- */

typedef struct {
    ui8 r, g, b, a;
} OBZ_Color;

void
obz_draw_circle(OBZ_Context *ctx,
                   const OBZ_Circle *c,
                   OBZ_Color color);

void
obz_draw_line(OBZ_Context *ctx,
                 OBZ_Point a,
                 OBZ_Point b,
                 OBZ_Color color);

/* Polygon (simple convex) */

typedef struct {
    const OBZ_Point *points;
    int count;
} OBZ_Polygon;

void
obz_draw_polygon(OBZ_Context *ctx,
                    const OBZ_Polygon *poly,
                    OBZ_Color color);

#ifdef __cplusplus
}
#endif
#endif
