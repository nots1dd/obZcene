#ifndef SDL_API_H
#define SDL_API_H

#include "SDL/Camera/camera.h"
#include "SDL/Render/render.h"
#include "SDL/Scene/scene.h"
#include "obz_log.h"
#include "types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stddef.h>
#include <stdint.h>

#define OBZ_EVENT_TIMER (SDL_USEREVENT + 1)

#ifdef __cplusplus
extern "C"
{
#endif

  struct OBZ_Window
  {
    SDL_Window*   win;
    SDL_Renderer* ren;
    SDL_Texture*  tex;
  };

  typedef struct
  {
    Uint64 last_counter;
    double perf_freq;
  } OBZ_SDL_HighResTimer;

  typedef struct
  {
    int width;
    int height;
  } OBZ_Dimensions;

  /* ---------- Error codes ---------- */
  typedef enum
  {
    OBZ_OK = 0,
    OBZ_ERR_SDL,
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

  /* ---------- Custom allocators ---------- */
  typedef void* (*OBZ_AllocFn)(size_t size);
  typedef void* (*OBZ_ReallocFn)(void* ptr, size_t size);
  typedef void (*OBZ_FreeFn)(void* ptr);

  typedef struct
  {
    OBZ_AllocFn   alloc;
    OBZ_ReallocFn realloc;
    OBZ_FreeFn    free;
  } OBZ_Allocator;

  /* ---------- Opaque handles ---------- */
  typedef struct OBZ_Context OBZ_Context;
  typedef struct OBZ_Window  OBZ_Window;

  /* ---------- Callback types ---------- */
  typedef void (*OBZ_UpdateFn)(OBZ_Context* ctx, float dt);
  typedef void (*OBZ_RenderFn)(OBZ_Context* ctx);
  typedef void (*OBZ_EventFn)(OBZ_Context* ctx, const void* event);

  typedef struct
  {
    OBZ_UpdateFn update;
    OBZ_RenderFn render;
    OBZ_EventFn  event;
  } OBZ_Callbacks;

  /* ---------- Window description ---------- */
  typedef struct
  {
    const char*    title;
    OBZ_Dimensions dim;
    OBZ_Bool       resizable;
  } OBZ_WindowDesc;

  /* ---------- Input state ---------- */
  typedef struct
  {
    int        mouse_x, mouse_y;
    int        mouse_dx, mouse_dy;
    OBZ_Bool   mouse_left;
    OBZ_Bool   mouse_right;
    const ui8* keyboard; /* SDL scancodes */
  } OBZ_InputState;

  /* ---------- API ---------- */
  OBZ_Context* obz_create(const OBZ_Callbacks* cb, const OBZ_Dimensions dims,
                          const OBZ_Allocator* alloc);

  void obz_destroy(OBZ_Context* ctx);

  OBZ_Result obz_window_create(OBZ_Context* ctx);

  void obz_window_destroy(OBZ_Window* win);

  OBZ_Result obz_run(OBZ_Context* ctx);

  void obz_request_quit(OBZ_Context* ctx);

  /* Accessors */
  OBZ_InputState* obz_input(OBZ_Context* ctx);

  OBZ_Window* obz_main_window(OBZ_Context* ctx);

  /* ---------- Timer ---------- */
  typedef int OBZ_TimerID;

  typedef ui32 (*OBZ_TimerFn)(OBZ_Context* ctx, ui32 interval_ms);

  OBZ_TimerID obz_add_timer(OBZ_Context* ctx, ui32 interval_ms, OBZ_TimerFn fn);

  void obz_remove_timer(OBZ_Context* ctx, OBZ_TimerID id);

  /* ---------- Internal structs ---------- */
  struct OBZ_Context
  {
    OBZ_Callbacks cb;
    OBZ_Allocator alloc;
    OBZ_Window*   main_win;

    OBZ_WindowDesc*      win_desc;
    OBZ_RendererContext* renctx;
    OBZ_SDL_HighResTimer timer;

    OBZ_InputState input;
    ui32           last_time;
    OBZ_Bool       quit;
    OBZ_Logger*    log;

    SDL_TimerID timers_id[32];
    OBZ_TimerFn timers_fn[32];
    ui32        timers_interval[32];
    int         timers_count;

    OBZ_Camera cam;
    OBZ_Scene* scene;
  };

#ifdef __cplusplus
}
#endif
#endif
