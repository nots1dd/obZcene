#include "Math/clamp.h"
#include "SDL/core.h"
#include "SDL/keymaps.h"
#include "obZcene.h"

DECLARE_OBZ_GLOBAL_LOGGER();

// you dont have to modify main.c AT ALL. Check out obZscene.h!

static void render(OBZ_Context* ctx)
{
  obz_render_clear(ctx->renctx, 0);

  OBZ_SceneEntry it;
  obz_scene_iter_begin(ctx->scene, &it);
  while (obz_scene_iter_next(ctx->scene, &it))
  {
    OBZ_Mesh3D* mesh_i = it.mesh;

    obz_render_mesh_camera(ctx->renctx, *it.pos, mesh_i, it.rot->x, it.rot->y, it.rot->z, ctx->cam);
  }

  // CROSSHAIRS (CUSTOM!)
  draw_crosshair(ctx->renctx, W / 2, H / 2, 20, 2, 0xFFFFFF);

  //OBZ_LOG_TRACE(ctx->log, "Frame rendered.");
}

static void update(OBZ_Context* ctx, float dt)
{
  if (!ctx || !ctx->scene)
    return;

  obz_scene_update(ctx->scene, dt);

  /* input handling lives here */
  (void)dt;
  OBZ_InputState* in = obz_input(ctx);
  (void)in;
}

void camera_move(OBZ_Camera* cam, const ui8* keyboard, float speed)
{
  Vec3 input = {0, 0, 0};

  if (keyboard[KC_W])
    input.z += 1;
  if (keyboard[KC_S])
    input.z -= 1;
  if (keyboard[KC_D])
    input.x += 1;
  if (keyboard[KC_A])
    input.x -= 1;
  if (keyboard[KC_SPACE])
    input.y += 1;
  if (keyboard[KC_BACKSPACE])
    input.y -= 1;

  if (obz_vec3_len(input) < 1e-6f)
  {
    cam->velocity = obz_vec3(0, 0, 0);
    return;
  }

  // Normalize input to avoid diagonal speed boost
  input = obz_vec3_norm(input);

  Vec3 fwd   = obz_vec3_norm(cam->direction);
  Vec3 right = obz_vec3_norm(obz_vec3_cross((Vec3){0, 1, 0}, fwd));
  if (obz_vec3_len(right) < 1e-6f)
    right = obz_vec3_norm(obz_vec3_cross((Vec3){0, 0, 1}, fwd));
  Vec3 up = obz_vec3_cross(fwd, right);

  // Compute velocity
  cam->velocity = obz_vec3_add(
    obz_vec3_add(obz_vec3_mulf(fwd, input.z * speed), obz_vec3_mulf(right, input.x * speed)),
    obz_vec3_mulf(up, input.y * speed));
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

  // Mouse look
  static int last_mx = 400, last_my = 300;
  int        dx = in->mouse_x - last_mx;
  int        dy = in->mouse_y - last_my;

  if (in->mouse_left) // Only rotate when left mouse held
  {
    float sensitivity = 0.006f;
    ctx->cam.yaw += dx * sensitivity;
    ctx->cam.pitch -= dy * sensitivity;

    // Clamp pitch
    ctx->cam.pitch = clampf(ctx->cam.pitch, -1.5f, 1.5f);
  }

  last_mx = in->mouse_x;
  last_my = in->mouse_y;

  // Update camera forward direction after mouse look
  obz_camera_update_direction(&ctx->cam);

  // Camera movement
  float speed = keyboard[KC_RSHIFT] ? 10.0f : 5.0f;
  camera_move(&ctx->cam, keyboard, speed);

  // Apply movement (use real frame delta if available)
  float dt = 0.016f; // ~60 FPS; replace with real frame delta if you have it
  obz_camera_update(&ctx->cam, dt);
}

int main(int argc, char** argv)
{
  OBZ_Callbacks cb  = {update, render, event};
  OBZ_Context*  ctx = obz_create(&cb, (OBZ_Dimensions){(int)W, (int)H}, NULL);

  ctx->cam                = obz_camera_init((Vec3){0, 0, 0}, W, H, 75.0f);
  ctx->timer.perf_freq    = SDL_GetPerformanceFrequency();
  ctx->timer.last_counter = SDL_GetPerformanceCounter();
  obz_camera_set_bounds(&ctx->cam, -W / 2 + margin, W / 2 - margin, -H / 2 + margin, H / 2 - margin,
                        -D / 2 + margin, D / 2 - margin);

  ctx->win_desc =
    &(OBZ_WindowDesc){.title      = "3D Room Demo - WASD + Mouse to move, Hold Left Click to look",
                      .dim.width  = W,
                      .dim.height = H,
                      .resizable  = OBZ_TRUE};

  obz_window_create(ctx);
  ctx->scene = obz_scene_create();

  OBZ_LOG_INFO(NULL, "obZcene 3D Room Demo started.");

  // Initialize scene
  init_scene(ctx->scene);
  obz_run(ctx);

  // Cleanup
  obz_destroy(ctx);

  return 0;
}
