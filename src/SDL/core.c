#include "SDL/core.h"
#include <stdarg.h>
#include <stdio.h>

void obz_set_logger(OBZ_Context *ctx, OBZ_LogFn fn)
{
    if (ctx) ctx->logger = fn;
}


void obz_log(OBZ_Context *ctx, const char *fmt, ...)
{
    if (!ctx || !fmt || !ctx->logger) return;

    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    ctx->logger(ctx, buf);
}

static ui32 timer_thread_trampoline(ui32 interval, void *userdata)
{
    OBZ_Context *ctx = userdata;

    SDL_Event ev;
    SDL_memset(&ev, 0, sizeof(ev));
    ev.type = OBZ_EVENT_TIMER;
    ev.user.code = 0;

    SDL_PushEvent(&ev);

    return interval; /* repeat */
}

OBZ_TimerID obz_add_timer(OBZ_Context *ctx,
                                ui32 interval_ms,
                                OBZ_TimerFn fn)
{
    if (!ctx || !fn || ctx->timers_count >= 32)
        return 0;

    SDL_TimerID id =
        SDL_AddTimer(interval_ms, timer_thread_trampoline, ctx);
    if (!id)
        return 0;

    int i = ctx->timers_count++;
    ctx->timers_id[i] = id;
    ctx->timers_fn[i] = fn;
    ctx->timers_interval[i] = interval_ms;

    return id;
}

void obz_remove_timer(OBZ_Context *ctx, OBZ_TimerID id)
{
    if (!ctx) return;

    for (int i = 0; i < ctx->timers_count; i++) {
        if (ctx->timers_id[i] == id) {
            SDL_RemoveTimer(id);
            ctx->timers_id[i] = ctx->timers_id[ctx->timers_count - 1];
            ctx->timers_fn[i] = ctx->timers_fn[ctx->timers_count - 1];
            ctx->timers_interval[i] =
                ctx->timers_interval[ctx->timers_count - 1];
            ctx->timers_count--;
            return;
        }
    }
}

/* ----- Alloc helpers ----- */
static void *A_malloc(const OBZ_Allocator *a, size_t s) {
    return a->alloc ? a->alloc(s) : malloc(s);
}
static void A_free(const OBZ_Allocator *a, void *p) {
    if (a->free) a->free(p); else free(p);
}

/* ----- Create ----- */
OBZ_Context *obz_create(const OBZ_Callbacks *cb,
                              const OBZ_Allocator *alloc)
{
    if (!cb) return NULL;

    /* --- SDL init --- */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
        return NULL;

    /* --- Global SDL hints --- */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");   /* linear filtering */
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    
    // Initialize PNG loader
    int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(flags) & flags) != flags) {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        return 0;
    }

#ifdef _WIN32
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

    OBZ_Context *ctx =
        alloc ? alloc->alloc(sizeof(*ctx)) : malloc(sizeof(*ctx));
    if (!ctx) return NULL;

    ctx->cb = *cb;
    ctx->alloc = alloc ? *alloc : (OBZ_Allocator){0};
    ctx->main_win = NULL;
    ctx->quit = OBZ_FALSE;
    ctx->last_time = SDL_GetTicks();
    ctx->input.keyboard = SDL_GetKeyboardState(NULL);

    return ctx;
}

/* ----- Destroy ----- */
void obz_destroy(OBZ_Context *ctx)
{
    if (!ctx) return;
    if (ctx->main_win) {
        SDL_DestroyRenderer(ctx->main_win->ren);
        SDL_DestroyWindow(ctx->main_win->win);
        A_free(&ctx->alloc, ctx->main_win);
    }
    IMG_Quit();
    SDL_Quit();
    A_free(&ctx->alloc, ctx);
}

/* ----- Create window ----- */
OBZ_Result obz_window_create(OBZ_Context *ctx)
{
    if (!ctx) return OBZ_ERR_INVALID;

    Uint32 flags = ctx->win_desc->resizable ? SDL_WINDOW_RESIZABLE : 0;

    SDL_Window *w = SDL_CreateWindow(
        ctx->win_desc->title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        ctx->win_desc->width, ctx->win_desc->height,
        flags
    );
    if (!w) return OBZ_ERR_SDL;

    SDL_Renderer *r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        SDL_DestroyWindow(w);
        return OBZ_ERR_SDL;
    }

    OBZ_Window *win = A_malloc(&ctx->alloc, sizeof(*win));
    if (!win) {
        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
        return OBZ_ERR_ALLOC;
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(r, ctx->win_desc->width, ctx->win_desc->height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    win->win = w;
    win->ren = r;
    ctx->main_win = win;


    return OBZ_OK;
}

/* ----- Destroy window ----- */
void obz_window_destroy(OBZ_Window *win)
{
    if (!win) return;
    SDL_DestroyRenderer(win->ren);
    SDL_DestroyWindow(win->win);
    /* caller frees window in destroy(ctx) */
}

/* ----- Input accessor ----- */
OBZ_InputState *obz_input(OBZ_Context *ctx)
{
    return ctx ? &ctx->input : NULL;
}

/* ----- Main window accessor ----- */
OBZ_Window *obz_main_window(OBZ_Context *ctx)
{
    return ctx ? ctx->main_win : NULL;
}

/* ----- Quit request ----- */
void obz_request_quit(OBZ_Context *ctx)
{
    ctx->quit = OBZ_TRUE;
}

/* ----- Run loop ----- */
OBZ_Result
obz_run(OBZ_Context *ctx)
{
    if (!ctx || !ctx->main_win) return OBZ_ERR_INVALID;

    SDL_Event ev;

    while (!ctx->quit) {
        /* Input reset */
        ctx->input.mouse_dx = ctx->input.mouse_dy = 0;

        /* Events */
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    ctx->quit = OBZ_TRUE;
                    break;
                case SDL_MOUSEMOTION:
                    ctx->input.mouse_x = ev.motion.x;
                    ctx->input.mouse_y = ev.motion.y;
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
                    for (int i = 0; i < ctx->timers_count; i++) {
                        ui32 next = ctx->timers_fn[i](ctx, ctx->timers_interval[i]);
                        if (next != ctx->timers_interval[i]) {
                            SDL_RemoveTimer(ctx->timers_id[i]);
                            ctx->timers_id[i] =
                                SDL_AddTimer(next, timer_thread_trampoline, ctx);
                            ctx->timers_interval[i] = next;
                        }
                    }
                    break;
            }

            if (ctx->cb.event)
                ctx->cb.event(ctx, &ev);
        }
        
        /* Delta time */
        ui32 now = SDL_GetTicks();
        float dt = (now - ctx->last_time) / 1000.0f;
        ctx->last_time = now;

        if (ctx->cb.update)
            ctx->cb.update(ctx, dt);

        /* Render */
        SDL_Renderer *r = ctx->main_win->ren;
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);

        if (ctx->cb.render)
            ctx->cb.render(ctx);

        SDL_RenderPresent(r);
    }

    return OBZ_OK;
}

void obz_renderer_clear(OBZ_Context *ctx, ui8 r, ui8 g, ui8 b)
{
    SDL_Renderer *ren = ctx->main_win->ren;
    SDL_SetRenderDrawColor(ren, r, g, b, 255);
    SDL_RenderClear(ren);
}

void obz_draw_pixel(OBZ_Context *ctx,
                               int x, int y,
                               ui8 r, ui8 g, ui8 b, ui8 a)
{
    SDL_Renderer *ren = ctx->main_win->ren;
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_RenderDrawPoint(ren, x, y);
}
