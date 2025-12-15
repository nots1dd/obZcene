#include "Math/clamp.h"
#include "SDL/core.h"
#include "obZcene.h"

DECLARE_OBZ_GLOBAL_LOGGER();

static OBZ_CamRotation mesh_rot = {0, 0, 0};

// you dont have to modify main.c AT ALL. Check out obZscene.h!
static void render(OBZ_Context* ctx)
{
  obz_render_clear(ctx->renctx, ctx->renctx->background_color);

  OBZ_SceneEntry it;
  obz_scene_iter_begin(ctx->scene, &it);
  while (obz_scene_iter_next(ctx->scene, &it) != OBZ_ERR_SCENE)
  {
    OBZ_Mesh3D* mesh_i = it.mesh;
    mesh_rot           = (OBZ_CamRotation){it.rot->x, it.rot->y, it.rot->z};

    obz_render_mesh_camera(ctx->renctx, *it.pos, mesh_i, mesh_rot, ctx->cam);
  }

  // CROSSHAIRS (CUSTOM!)
  draw_crosshair(ctx->renctx, g_W / 2, g_H / 2, 20, 2, COLOR_GOLD);

  obz_arena_reset(ctx->renctx->arena_alloc);
}

static void update(OBZ_Context* ctx, float dt)
{
  if (!ctx || !ctx->scene)
    return;

  obz_scene_update(ctx->scene, dt);
  obz_camera_update(&ctx->cam, dt);

  //print_camera_debug(&ctx->cam);

  /* input handling lives here */
  (void)dt;
  OBZ_InputState* in = obz_input(ctx);
  (void)in;
}

static void event(OBZ_Context* ctx, const void* ev)
{
  OBZ_InputState* in       = obz_input(ctx);
  const ui8*      keyboard = in->keyboard;

  // Escape to quit
  if (keyboard[KC_ESCAPE])
  {
    OBZ_LOG_INFO(NULL, "Escape pressed, quitting...");
    obz_request_quit(ctx);
    return;
  }
  // pressing B
  if (keyboard[KC_B])
  {
    if (ctx->renctx->background_color == COLOR_WHITE_PIXELS)
      ctx->renctx->background_color = COLOR_BLACK_PIXELS;
    else
      ctx->renctx->background_color = COLOR_WHITE_PIXELS;
  }

  // Mouse look
  static int last_mx = 400, last_my = 300;
  int        dx = in->mouse_x - last_mx;
  int        dy = in->mouse_y - last_my;

  if (in->mouse_left) // Only rotate when left mouse held
  {
    float sensitivity = 0.006f;
    ctx->cam.rot.yaw += dx * sensitivity;
    ctx->cam.rot.pitch -= dy * sensitivity;

    // Clamp pitch
    ctx->cam.rot.pitch = obz_clampf(ctx->cam.rot.pitch, -1.5f, 1.5f);
  }

  last_mx = in->mouse_x;
  last_my = in->mouse_y;

  // Update camera forward direction after mouse look
  obz_camera_update_direction(&ctx->cam);

  // Camera movement
  float speed = keyboard[KC_RSHIFT] ? 10.0f : 5.0f;
  camera_move(&ctx->cam, keyboard, speed);
}

int main(int argc, char** argv)
{
  OBZ_Callbacks cb  = {update, render, event};
  OBZ_Context*  ctx = obz_create(&cb, (OBZ_Dimensions){(int)g_W, (int)g_H}, NULL);

  ctx->cam                = obz_camera_init(g_camera_world_init, g_W, g_H, 75.0f);
  ctx->timer.perf_freq    = SDL_GetPerformanceFrequency();
  ctx->timer.last_counter = SDL_GetPerformanceCounter();
  obz_camera_set_bounds(&ctx->cam, g_x_axis, g_y_axis, g_z_axis);

  ctx->win_desc =
    &(OBZ_WindowDesc){.title      = "3D Room Demo - WASD + Mouse to move, Hold Left Click to look",
                      .dim.width  = ctx->renctx->width,
                      .dim.height = ctx->renctx->height,
                      .resizable  = OBZ_TRUE};

  obz_window_create(ctx);
  ctx->scene = obz_scene_create();

  OBZ_LOG_INFO(NULL, "obZcene 3D Room Demo started.");

  // Initialize scene
  init_scene(ctx->scene, argv[1]);
  obz_run(ctx);

  // Cleanup
  obz_destroy(ctx);

  return OBZ_OK;
}
