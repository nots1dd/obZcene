#include "SDL/shapes.h"
#include "SDL/macros.h"
#include <stdlib.h>

static OBZ_FORCE_INLINE void draw_pixel(OBZ_Context *ctx, int x, int y, OBZ_Color c) {
    obz_draw_pixel(ctx, x, y, c.r, c.g, c.b, c.a);
}

/* ============================================================
   LINE (Bresenham)
   ============================================================ */
void obz_draw_line(OBZ_Context *ctx,
                      OBZ_Point a,
                      OBZ_Point b,
                      OBZ_Color c)
{
    int x0 = a.x, y0 = a.y;
    int x1 = b.x, y1 = b.y;

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (1) {
        draw_pixel(ctx, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* ============================================================
   CIRCLE (Midpoint Algorithm)
   ============================================================ */
void obz_draw_circle(OBZ_Context *ctx,
                        const OBZ_Circle *c,
                        OBZ_Color col)
{
    int x0 = c->cx;
    int y0 = c->cy;
    int r  = c->radius;

    int x = r;
    int y = 0;
    int err = 1 - x;

    while (x >= y) {
        draw_pixel(ctx, x0 + x, y0 + y, col);
        draw_pixel(ctx, x0 + y, y0 + x, col);
        draw_pixel(ctx, x0 - y, y0 + x, col);
        draw_pixel(ctx, x0 - x, y0 + y, col);
        draw_pixel(ctx, x0 - x, y0 - y, col);
        draw_pixel(ctx, x0 - y, y0 - x, col);
        draw_pixel(ctx, x0 + y, y0 - x, col);
        draw_pixel(ctx, x0 + x, y0 - y, col);

        y++;
        if (err < 0) {
            err += 2*y + 1;
        } else {
            x--;
            err += 2*(y - x + 1);
        }
    }
}

/* ============================================================
   POLYGON (simple line loop)
   ============================================================ */
void obz_draw_polygon(OBZ_Context *ctx,
                         const OBZ_Polygon *poly,
                         OBZ_Color color)
{
    if (!poly || poly->count < 2) return;

    const OBZ_Point *p = poly->points;

    for (int i = 0; i < poly->count - 1; i++)
        obz_draw_line(ctx, p[i], p[i+1], color);

    /* close shape */
    obz_draw_line(ctx, p[poly->count - 1], p[0], color);
}
