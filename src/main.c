#include "SDL/Mesh/3dshapes.h"
#include "SDL/Render/render.h"
#include "SDL/Scene/scene.h"
#include "SDL/core.h"
#include "SDL/keymaps.h"

static const float W      = 1000;
static const float H      = 400;
static const float D      = 1000;
static const float margin = 20.0f; // distance from wall

static OBZ_Mesh3D*  cube     = NULL;
static OBZ_Mesh3D*  sphere   = NULL;
static OBZ_Mesh3D*  pyramid  = NULL;
static OBZ_Mesh3D*  quad     = NULL;
static OBZ_Texture* quad_tex = NULL;

static void init_scene(void)
{
  cube     = malloc(sizeof(OBZ_Mesh3D));
  *cube    = make_cube(100);
  sphere   = malloc(sizeof(OBZ_Mesh3D));
  *sphere  = make_sphere(50, 12);
  pyramid  = malloc(sizeof(OBZ_Mesh3D));
  *pyramid = make_pyramid(80, 120);
  quad     = malloc(sizeof(OBZ_Mesh3D));
  *quad    = make_quad(30, 30);

  int cube_id =
    scene_add_mesh_ptr(cube, (Vec3){0, 0, 0}, (Rot3){0, 0, 0}, COLOR_RED, 1, (Rot3){0, 0, 0});
  scene_init_textures_for_mesh(cube_id); // initialize textures for cube

  scene_add_room(W, H, D, 4, (Rot3){0, 0, 0}, (Rot3){0, 0, 0});

  /* corner pyramids */
  Vec3 pyramid_pos1 = {400, -150, -400};
  Vec3 pyramid_pos2 = {400, -150, 400};
  Vec3 pyramid_pos3 = {-400, -150, 400};
  Vec3 pyramid_pos4 = {-400, -150, -400};

  int pid1 =
    scene_add_pyramid(pyramid, pyramid_pos1, (Rot3){0, 0, 0}, (Rot3){15, 30, 45}, COLOR_BLUE);
  scene_init_textures_for_mesh(pid1);
  int pid2 =
    scene_add_pyramid(pyramid, pyramid_pos2, (Rot3){0, 0, 0}, (Rot3){15, 30, 45}, COLOR_MAGENTA);
  scene_init_textures_for_mesh(pid2);
  int pid3 =
    scene_add_pyramid(pyramid, pyramid_pos3, (Rot3){0, 0, 0}, (Rot3){15, 30, 45}, COLOR_LIME);
  scene_init_textures_for_mesh(pid3);
  int pid4 =
    scene_add_pyramid(pyramid, pyramid_pos4, (Rot3){0, 0, 0}, (Rot3){15, 30, 45}, COLOR_WHITE);
  scene_init_textures_for_mesh(pid4);

  int sid = scene_add_sphere(sphere, 200, 60, (Rot3){0, 0, 0}, (Rot3){0, 60, 0}, COLOR_YELLOW);
  scene_init_textures_for_mesh(sid);

  /* quad */
  int qid =
    scene_add_mesh_ptr(quad, (Vec3){100, 100, 0}, (Rot3){0, 0, 0}, COLOR_WHITE, 1, (Rot3){0, 0, 0});
  scene_init_textures_for_mesh(qid);

  // Load texture for quad
  quad_tex = obz_tex_load_png("assets/obzen.png");
  scene_bind_texture_to_mesh(qid, quad_tex, 0);
}

static void render(OBZ_Context* ctx)
{
  obz_renderer_clear(ctx->renctx, 15, 15, 25, 0);

  scene_update(0.012f); // ~90fps

  for (int i = 0; i < g_scene.count; i++)
  {
    OBZ_Mesh3D*       mesh_i       = g_scene.meshes[i];
    OBZ_MeshTextures* mesh_texture = g_scene.textures[i];
    OBZ_Color         mesh_color   = g_scene.colors[i];

    obz_draw_mesh_textured_camera(ctx->renctx, g_scene.positions[i], mesh_i, mesh_texture,
                                  mesh_color, g_scene.rotations[i].x, g_scene.rotations[i].y,
                                  g_scene.rotations[i].z, ctx->cam);
  }

  // CROSSHAIRS
  obz_draw_line(ctx->renctx, (Vec2){W / 2 - 10, H / 2}, (Vec2){W / 2 + 10, H / 2}, COLOR_WHITE);
  obz_draw_line(ctx->renctx, (Vec2){W / 2, H / 2 - 10}, (Vec2){W / 2, H / 2 + 10}, COLOR_WHITE);
}

static void update(OBZ_Context* ctx, float dt)
{
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

static void event(OBZ_Context* ctx, const void* ev)
{
  const ui8*      keyboard = obz_input(ctx)->keyboard;
  OBZ_InputState* in       = obz_input(ctx);

  // Escape to quit
  if (keyboard[KC_ESCAPE])
  {
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

static void logger(OBZ_Context* ctx, const char* msg)
{
  (void)ctx;
  printf("[LOG] %s\n", msg);
}

int main(void)
{
  OBZ_Callbacks cb  = {update, render, event};
  OBZ_Context*  ctx = obz_create(&cb, (OBZ_Dimensions){(int)W, (int)H}, NULL);

  ctx->cam = obz_camera_init((Vec3){0, 0, 0}, W, H, 75.0f);
  obz_camera_set_bounds(&ctx->cam, -W / 2 + margin, W / 2 - margin, -H / 2 + margin, H / 2 - margin,
                        -D / 2 + margin, D / 2 - margin);
  obz_set_logger(ctx, logger);

  ctx->win_desc =
    &(OBZ_WindowDesc){.title     = "3D Room Demo - WASD + Mouse to move, Hold Left Click to look",
                      .width     = W,
                      .height    = H,
                      .resizable = OBZ_TRUE};

  obz_window_create(ctx);

  // Initialize scene
  init_scene();
  obz_run(ctx);

  // Cleanup
  obz_destroy(ctx);

  return 0;
}
