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
    OBZ_Mesh3D*       mesh_i       = it.mesh;
    OBZ_MeshTextures* mesh_texture = obz_scene_get_mesh_textures(ctx->scene, it.index);

    obz_render_mesh_textured_camera(ctx->renctx, *it.pos, mesh_i, mesh_texture, it.rot->x, it.rot->y,
                                  it.rot->z, ctx->cam);
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

void move_camera_input(OBZ_Camera* cam, const ui8* keyboard, float speed)
{
  float f = 0, r = 0, u = 0;

  // WASD
  if (keyboard[KC_W])
    f += 1;
  if (keyboard[KC_S])
    f -= 1;
  if (keyboard[KC_D])
    r += 1;
  if (keyboard[KC_A])
    r -= 1;

  // Vertical
  if (keyboard[KC_SPACE])
    u += 1;
  if (keyboard[KC_BACKSPACE])
    u -= 1;

  // Normalize direction
  Vec3 forward = cam->direction;
  obz_vec3_norm(forward);

  // Build right and up vectors
  Vec3 world_up = {0, 1, 0};
  Vec3 rightv   = obz_vec3_cross(world_up, forward);
  obz_vec3_norm(rightv);

  Vec3 upv = obz_vec3_cross(forward, rightv);

  // Compute velocity
  cam->velocity.x = forward.x * f * speed + rightv.x * r * speed + upv.x * u * speed;
  cam->velocity.y = forward.y * f * speed + rightv.y * r * speed + upv.y * u * speed;
  cam->velocity.z = forward.z * f * speed + rightv.z * r * speed + upv.z * u * speed;
}

static void event(OBZ_Context* ctx, [[maybe_unused]] const void* ev)
{
  const ui8*      keyboard = obz_input(ctx)->keyboard;
  OBZ_InputState* in       = obz_input(ctx);

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

  if (in->mouse_left)
  { // Only rotate when left mouse held
    float sensitivity = 0.006f;
    ctx->cam.yaw += dx * sensitivity;
    ctx->cam.pitch -= dy * sensitivity;

    // Clamp pitch
    if (ctx->cam.pitch > 1.5f)
      ctx->cam.pitch = 1.5f;
    if (ctx->cam.pitch < -1.5f)
      ctx->cam.pitch = -1.5f;
  }

  last_mx = in->mouse_x;
  last_my = in->mouse_y;

  // Camera movement (faster with shift)
  float speed = keyboard[KC_RSHIFT] ? 8.0f : 3.0f;
  obz_camera_update_direction(&ctx->cam); // if yaw/pitch changed
  move_camera_input(&ctx->cam, keyboard, speed);
  obz_camera_update(&ctx->cam, 1); // applies cam->velocity
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
