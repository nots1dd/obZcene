#ifndef SDL_API_H
#define SDL_API_H

#include <stddef.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL/camera.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MHMAPI_EVENT_TIMER (SDL_USEREVENT + 1)

/* ---------- Error codes ---------- */
typedef enum {
    MHMAPI_OK = 0,
    MHMAPI_ERR_SDL,
    MHMAPI_ERR_ALLOC,
    MHMAPI_ERR_INVALID,
    MHMAPI_ERR_RUNTIME
} MHMAPI_Result;

/* ---------- Bool ---------- */
typedef enum {
    MHMAPI_FALSE = 0,
    MHMAPI_TRUE  = 1
} MHMAPI_Bool;

/* ---------- Custom allocators ---------- */
typedef void *(*MHMAPI_AllocFn)(size_t size);
typedef void *(*MHMAPI_ReallocFn)(void *ptr, size_t size);
typedef void  (*MHMAPI_FreeFn)(void *ptr);

typedef struct {
    MHMAPI_AllocFn   alloc;
    MHMAPI_ReallocFn realloc;
    MHMAPI_FreeFn    free;
} MHMAPI_Allocator;

/* ---------- Opaque handles ---------- */
typedef struct MHMAPI_Context MHMAPI_Context;
typedef struct MHMAPI_Window  MHMAPI_Window;

/* ---------- Callback types ---------- */
typedef void (*MHMAPI_UpdateFn)(MHMAPI_Context *ctx, float dt);
typedef void (*MHMAPI_RenderFn)(MHMAPI_Context *ctx);
typedef void (*MHMAPI_EventFn) (MHMAPI_Context *ctx, const void *event);

typedef struct {
    MHMAPI_UpdateFn update;
    MHMAPI_RenderFn render;
    MHMAPI_EventFn  event;
} MHMAPI_Callbacks;

/* ---------- Window description ---------- */
typedef struct {
    const char *title;
    int width;
    int height;
    MHMAPI_Bool resizable;
} MHMAPI_WindowDesc;

/* ---------- Input state ---------- */
typedef struct {
    int mouse_x, mouse_y;
    int mouse_dx, mouse_dy;
    MHMAPI_Bool mouse_left;
    MHMAPI_Bool mouse_right;
    const ui8 *keyboard; /* SDL scancodes */
} MHMAPI_InputState;

/* ---------- API ---------- */
MHMAPI_Context *
mhmapi_create(const MHMAPI_Callbacks *cb,
              const MHMAPI_Allocator *alloc);

void
mhmapi_destroy(MHMAPI_Context *ctx);

MHMAPI_Result
mhmapi_window_create(MHMAPI_Context *ctx);

void
mhmapi_window_destroy(MHMAPI_Window *win);

MHMAPI_Result
mhmapi_run(MHMAPI_Context *ctx);

void
mhmapi_request_quit(MHMAPI_Context *ctx);

/* Accessors */
MHMAPI_InputState *
mhmapi_input(MHMAPI_Context *ctx);

MHMAPI_Window *
mhmapi_main_window(MHMAPI_Context *ctx);

/* ---------- Timer ---------- */
typedef int MHMAPI_TimerID;

typedef ui32 (*MHMAPI_TimerFn)(MHMAPI_Context *ctx, ui32 interval_ms);

MHMAPI_TimerID
mhmapi_add_timer(MHMAPI_Context *ctx,
                 ui32 interval_ms,
                 MHMAPI_TimerFn fn);

void
mhmapi_remove_timer(MHMAPI_Context *ctx, MHMAPI_TimerID id);

/* ---------- Logging ---------- */
typedef void (*MHMAPI_LogFn)(MHMAPI_Context *ctx,
                             const char *msg);

void
mhmapi_set_logger(MHMAPI_Context *ctx, MHMAPI_LogFn fn);

void
mhmapi_log(MHMAPI_Context *ctx, const char *fmt, ...);

void mhmapi_renderer_clear(MHMAPI_Context *ctx, ui8 r, ui8 g, ui8 b);
void mhmapi_draw_pixel(MHMAPI_Context *ctx,
                               int x, int y,
                               ui8 r, ui8 g, ui8 b, ui8 a);

/* ---------- Internal structs ---------- */
struct MHMAPI_Window {
    SDL_Window   *win;
    SDL_Renderer *ren;
};

typedef struct 
{
  int width;
  int height;
} Dimensions;

struct MHMAPI_Context {
    MHMAPI_Callbacks cb;
    MHMAPI_Allocator alloc;
    MHMAPI_Window *main_win;
    
    MHMAPI_WindowDesc *win_desc;

    MHMAPI_InputState input;
    ui32 last_time;
    MHMAPI_Bool quit;
    MHMAPI_LogFn logger;

    SDL_TimerID timers_id[32];
    MHMAPI_TimerFn timers_fn[32];
    ui32 timers_interval[32];
    int timers_count;

    Camera cam;
};

#ifdef __cplusplus
}
#endif
#endif
