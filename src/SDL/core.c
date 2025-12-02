#include "SDL/core.h"
#include <stdarg.h>
#include <stdio.h>

static ui32 timer_thread_trampoline(ui32 interval, void* userdata)
{
  [[maybe_unused]] OBZ_Context* ctx = userdata;

  SDL_Event ev;
  SDL_memset(&ev, 0, sizeof(ev));
  ev.type      = OBZ_EVENT_TIMER;
  ev.user.code = 0;

  SDL_PushEvent(&ev);

  return interval; /* repeat */
}

OBZ_TimerID obz_add_timer(OBZ_Context* ctx, ui32 interval_ms, OBZ_TimerFn fn)
{
  if (!ctx || !fn || ctx->timers_count >= 32)
    return 0;

  SDL_TimerID id = SDL_AddTimer(interval_ms, timer_thread_trampoline, ctx);
  if (!id)
    return 0;

  int i                   = ctx->timers_count++;
  ctx->timers_id[i]       = id;
  ctx->timers_fn[i]       = fn;
  ctx->timers_interval[i] = interval_ms;

  return id;
}

void obz_remove_timer(OBZ_Context* ctx, OBZ_TimerID id)
{
  if (!ctx)
    return;

  for (int i = 0; i < ctx->timers_count; i++)
  {
    if (ctx->timers_id[i] == id)
    {
      SDL_RemoveTimer(id);
      ctx->timers_id[i]       = ctx->timers_id[ctx->timers_count - 1];
      ctx->timers_fn[i]       = ctx->timers_fn[ctx->timers_count - 1];
      ctx->timers_interval[i] = ctx->timers_interval[ctx->timers_count - 1];
      ctx->timers_count--;
      return;
    }
  }
}

/* ----- Alloc helpers ----- */
static void* A_malloc(const OBZ_Allocator* a, size_t s)
{
  return a->alloc ? a->alloc(s) : obz_malloc(s);
}
static void A_free(const OBZ_Allocator* a, void* p)
{
  if (a->free)
    a->free(p);
  else
    obz_free(p);
}

/* ----- Create ----- */
OBZ_Context* obz_create(const OBZ_Callbacks* cb, const OBZ_Dimensions dims,
                        const OBZ_Allocator* alloc)
{
  if (!cb)
  {
    fprintf(stderr, "[ERROR] Callback struct is NULL.\n");
    return NULL;
  }

  OBZ_Context* ctx = alloc ? alloc->alloc(sizeof(*ctx)) : obz_malloc(sizeof(*ctx));

  ctx->cb             = *cb;
  ctx->alloc          = alloc ? *alloc : (OBZ_Allocator){0};
  ctx->main_win       = NULL;
  ctx->quit           = OBZ_FALSE;
  ctx->last_time      = SDL_GetTicks();
  ctx->input.keyboard = SDL_GetKeyboardState(NULL);
  ctx->log            = obz_logger_init(NULL, OBZ_LOG_TRACE);
  OBZ_LOG_INFO(ctx->log, "obZcene logger created successfully!");
  obz_set_global_logger(ctx->log);
  // will check for NULL internally
  ctx->renctx = obz_render_context_init(dims.width, dims.height);
  OBZ_LOG_INFO(ctx->log, "obZcene context created successfully!");
  OBZ_LOG_DEBUG(NULL, "Initializing SDL...");

  /* --- SDL init --- */
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
  {
    OBZ_LOG_ERROR(NULL, "SDL_Init failed: %s", SDL_GetError());
    return NULL;
  }
  OBZ_LOG_DEBUG(NULL, "SDL initialized successfully.");

  /* --- Global SDL hints --- */
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); /* linear filtering */
  SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
  OBZ_LOG_TRACE(NULL, "SDL hints set.");

  // Initialize PNG/JPG loader
  int flags = IMG_INIT_PNG | IMG_INIT_JPG;
  if ((IMG_Init(flags) & flags) != flags)
  {
    OBZ_LOG_ERROR(NULL, "IMG_Init failed: %s", IMG_GetError());
    return 0;
  }
  OBZ_LOG_DEBUG(NULL, "SDL_image initialized with PNG & JPG support.");

#ifdef _WIN32
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

  return ctx;
}

/* ----- Destroy ----- */
void obz_destroy(OBZ_Context* ctx)
{
  if (!ctx)
    return;

  OBZ_LOG_INFO(NULL, "Destroying obZcene scene and context...");

  if (ctx->main_win)
  {
    SDL_DestroyRenderer(ctx->main_win->ren);
    SDL_DestroyWindow(ctx->main_win->win);
    A_free(&ctx->alloc, ctx->main_win);
    OBZ_LOG_DEBUG(NULL, "Main window destroyed.");
  }

  IMG_Quit();
  SDL_Quit();
  OBZ_LOG_DEBUG(ctx->log, "SDL and SDL_image cleaned up.");

  if (ctx->renctx)
  {
    if (ctx->renctx->framebuffer)
      free(ctx->renctx->framebuffer);
    free(ctx->renctx);
    OBZ_LOG_DEBUG(ctx->log, "Renderer context freed.");
  }

  OBZ_LOG_INFO(NULL, "obZcene context destroyed.");
  obz_logger_destroy(ctx->log);

  A_free(&ctx->alloc, ctx);
}

/* ----- Create window ----- */
OBZ_Result obz_window_create(OBZ_Context* ctx)
{
  if (!ctx)
    return OBZ_ERR_INVALID;

  Uint32 flags = ctx->win_desc->resizable ? SDL_WINDOW_RESIZABLE : 0;

  SDL_Window* w =
    SDL_CreateWindow(ctx->win_desc->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                     ctx->win_desc->dim.width, ctx->win_desc->dim.height, flags);
  if (!w)
  {
    OBZ_LOG_ERROR(NULL, "SDL_CreateWindow failed: %s", SDL_GetError());
    return OBZ_ERR_SDL;
  }

  SDL_Renderer* r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
  if (!r)
  {
    SDL_DestroyWindow(w);
    OBZ_LOG_ERROR(NULL, "SDL_CreateRenderer failed: %s", SDL_GetError());
    return OBZ_ERR_SDL;
  }

  OBZ_Window* win = A_malloc(&ctx->alloc, sizeof(*win));
  if (!win)
  {
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    OBZ_LOG_ERROR(NULL, "Failed to allocate OBZ_Window.");
    return OBZ_ERR_ALLOC;
  }

  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_RenderSetLogicalSize(r, ctx->win_desc->dim.width, ctx->win_desc->dim.height);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

  win->win      = w;
  win->ren      = r;
  ctx->main_win = win;

  ctx->main_win->tex =
    SDL_CreateTexture(ctx->main_win->ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                      ctx->renctx->width, ctx->renctx->height);

  OBZ_LOG_INFO(NULL, "Window created successfully: %dx%d", ctx->win_desc->dim.width,
               ctx->win_desc->dim.height);

  return OBZ_OK;
}

/* ----- Destroy window ----- */
void obz_window_destroy(OBZ_Window* win)
{
  if (!win)
    return;

  OBZ_LOG_DEBUG(NULL, "Destroying window...");

  SDL_DestroyRenderer(win->ren);
  SDL_DestroyWindow(win->win);
  /* caller frees window in destroy(ctx) */
}

/* ----- Input accessor ----- */
OBZ_InputState* obz_input(OBZ_Context* ctx) { return ctx ? &ctx->input : NULL; }

/* ----- Main window accessor ----- */
OBZ_Window* obz_main_window(OBZ_Context* ctx) { return ctx ? ctx->main_win : NULL; }

/* ----- Quit request ----- */
void obz_request_quit(OBZ_Context* ctx) { ctx->quit = OBZ_TRUE; }

/* ----- Run loop ----- */
OBZ_Result obz_run(OBZ_Context* ctx)
{
  if (!ctx || !ctx->main_win)
    return OBZ_ERR_INVALID;

  SDL_Event ev;

  while (!ctx->quit)
  {
    /* Input reset */
    ctx->input.mouse_dx = ctx->input.mouse_dy = 0;

    /* Events */
    while (SDL_PollEvent(&ev))
    {
      switch (ev.type)
      {
        case SDL_QUIT:
          ctx->quit = OBZ_TRUE;
          break;
        case SDL_MOUSEMOTION:
          ctx->input.mouse_x  = ev.motion.x;
          ctx->input.mouse_y  = ev.motion.y;
          ctx->input.mouse_dx = ev.motion.xrel;
          ctx->input.mouse_dy = ev.motion.yrel;
          break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          if (ev.button.button == SDL_BUTTON_LEFT)
            ctx->input.mouse_left = (ev.button.state == SDL_PRESSED);
          else if (ev.button.button == SDL_BUTTON_RIGHT)
            ctx->input.mouse_right = (ev.button.state == SDL_PRESSED);
          break;
        case OBZ_EVENT_TIMER:
          for (int i = 0; i < ctx->timers_count; i++)
          {
            ui32 next = ctx->timers_fn[i](ctx, ctx->timers_interval[i]);
            if (next != ctx->timers_interval[i])
            {
              SDL_RemoveTimer(ctx->timers_id[i]);
              ctx->timers_id[i]       = SDL_AddTimer(next, timer_thread_trampoline, ctx);
              ctx->timers_interval[i] = next;
            }
          }
          break;
      }

      if (ctx->cb.event)
        ctx->cb.event(ctx, &ev);
    }

    /* Delta time */
    Uint64 now              = SDL_GetPerformanceCounter();
    float  dt               = (float)((now - ctx->timer.last_counter) / ctx->timer.perf_freq);
    ctx->timer.last_counter = now;

    if (ctx->cb.update)
      ctx->cb.update(ctx, dt);

    /* Render */
    SDL_Renderer* r = ctx->main_win->ren;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    if (ctx->cb.render)
      ctx->cb.render(ctx);

    SDL_UpdateTexture(ctx->main_win->tex, NULL, ctx->renctx->framebuffer, ctx->renctx->width * 4);
    SDL_RenderCopy(ctx->main_win->ren, ctx->main_win->tex, NULL, NULL);

    SDL_RenderPresent(r);
  }

  return OBZ_OK;
}
