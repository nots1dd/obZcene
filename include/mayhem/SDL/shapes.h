#ifndef MHMAPI_SHAPES_H
#define MHMAPI_SHAPES_H

#include <stdint.h>
#include "core.h"
#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Basic shapes ---------- */

typedef struct {
    int x, y;
} MHMAPI_Point;

typedef struct {
    int x, y, w, h;
} MHMAPI_Rect;

typedef struct {
    int cx, cy;
    int radius;
} MHMAPI_Circle;

/* ---------- Colors ---------- */

typedef struct {
    ui8 r, g, b, a;
} MHMAPI_Color;

void
mhmapi_draw_circle(MHMAPI_Context *ctx,
                   const MHMAPI_Circle *c,
                   MHMAPI_Color color);

void
mhmapi_draw_line(MHMAPI_Context *ctx,
                 MHMAPI_Point a,
                 MHMAPI_Point b,
                 MHMAPI_Color color);

/* Polygon (simple convex) */

typedef struct {
    const MHMAPI_Point *points;
    int count;
} MHMAPI_Polygon;

void
mhmapi_draw_polygon(MHMAPI_Context *ctx,
                    const MHMAPI_Polygon *poly,
                    MHMAPI_Color color);

#ifdef __cplusplus
}
#endif
#endif
