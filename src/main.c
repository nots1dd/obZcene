#include "SDL/camera.h"
#include "SDL/core.h"
#include "SDL/keymaps.h"
#include "SDL/scene.h"
#include "SDL/colors.h"
#include "SDL/shapes.h"
#include "SDL/render.h"

static const float W = 1000;
static const float H = 400;
static const float D = 1000;
static const float margin = 20.0f;   // distance from wall

static Mesh3D *sphere = NULL;
static Mesh3D *pyramid = NULL;

//static MHMAPI_Texture *quad_tex = NULL;

static void init_scene(void) {
    
    Mesh3D *cube = malloc(sizeof(Mesh3D));
    *cube = make_cube(100);

    sphere = malloc(sizeof(Mesh3D));
    *sphere = make_sphere(50, 12);

    pyramid = malloc(sizeof(Mesh3D));
    *pyramid = make_pyramid(80, 120);

    /* cube */
    // general use of mesh ptr
    scene_add_mesh_ptr(
        cube,
        (Vec3){0,0,0},
        (Rot3){0,0,0},
        COLOR_RED,
        1,
        (Rot3){0,0,0}
    );

    /* room */
    scene_add_room(
        W, H, D,     // width, height, depth
        4,                   // tiles (simple)
        (Rot3){0,0,0},
        (Rot3){0,0,0}
    );

    /* corner pyramids */
    Vec3 pyramid_pos1 = {400, -150, -400};
    Vec3 pyramid_pos2 = {400, -150, 400};
    Vec3 pyramid_pos3 = {-400, -150, 400};
    Vec3 pyramid_pos4 = {-400, -150, -400};

    scene_add_pyramid(
        pyramid,
        pyramid_pos1,
        (Rot3){0,0,0},
        (Rot3){15,30,45}
    );
    scene_add_pyramid(
        pyramid,
        pyramid_pos2,
        (Rot3){0,0,0},
        (Rot3){15,30,45}
    );
    scene_add_pyramid(
        pyramid,
        pyramid_pos3,
        (Rot3){0,0,0},
        (Rot3){15,30,45}
    );
    scene_add_pyramid(
        pyramid,
        pyramid_pos4,
        (Rot3){0,0,0},
        (Rot3){15,30,45}
    );

    scene_add_sphere(
        sphere,
        200,
        60,
        (Rot3){0,0,0},
        (Rot3){0,60,0}
    );

    //MHMAPI_MeshTextures *mt_quad = mhmapi_tex_create_for_mesh(quad);
    //mhmapi_tex_bind_to_mesh(mt_quad, quad_tex, 0);
}

static void render(MHMAPI_Context *ctx) {
    mhmapi_renderer_clear(ctx, 15,15,25);

    scene_update(0.011f); // ~90fps

    for (int i = 0; i < g_scene.count; i++) {
        Mesh3D *m = g_scene.meshes[i];   /* get pointer */

        draw_mesh_wire_camera_fast(
            ctx,
            g_scene.positions[i],
            m,                                  /* PASS Mesh3D* */
            g_scene.rotations[i].x,
            g_scene.rotations[i].y,
            g_scene.rotations[i].z,
            W, H,
            g_scene.colors[i],
            ctx->cam
        );

    }


    // CROSSHAIRS
    mhmapi_draw_line(ctx, 
        (MHMAPI_Point){W/2 - 10, H/2}, 
        (MHMAPI_Point){W/2 + 10, H/2}, 
        COLOR_WHITE);
    mhmapi_draw_line(ctx, 
        (MHMAPI_Point){W/2, H/2 - 10}, 
        (MHMAPI_Point){W/2, H/2 + 10}, 
        COLOR_WHITE);
}

static void update(MHMAPI_Context *ctx, float dt) {
    (void)dt;
    MHMAPI_InputState *in = mhmapi_input(ctx);
    (void)in;
}

void move_camera_input(Camera *cam, const ui8 *keyboard, float speed) {
    float forward = 0, right = 0, up = 0;
    
    // WASD movement
    if (keyboard[KC_W]) forward += 1;  // W
    if (keyboard[KC_S]) forward -= 1;  // S
    if (keyboard[KC_D])  right += 1;    // D
    if (keyboard[KC_A])  right -= 1;    // A
    
    // Vertical movement
    if (keyboard[KC_SPACE]) up += 1;
    if (keyboard[KC_BACKSPACE]) up -= 1;
    
    // Calculate movement vectors
    // float forward_x = __SIMD_sin_scalar_sse(cam->yaw);
    // float forward_z = __SIMD_cos_scalar_sse(cam->yaw);
    // float right_x = __SIMD_cos_scalar_sse(cam->yaw);
    // float right_z = -__SIMD_sin_scalar_sse(cam->yaw);
    
    float forward_x = sinf(cam->yaw);
    float forward_z = cosf(cam->yaw);
    float right_x = cosf(cam->yaw);
    float right_z = -sinf(cam->yaw);

    if (cam->position.x < -W/2 + margin) cam->position.x = -W/2 + margin;
    if (cam->position.x >  W/2 - margin) cam->position.x =  W/2 - margin;
    if (cam->position.z < -D/2 + margin) cam->position.z = -D/2 + margin;
    if (cam->position.z >  D/2 - margin) cam->position.z =  D/2 - margin;
    if (cam->position.y < -H/2 + margin) cam->position.y = -H/2 + margin;
    if (cam->position.y >  H/2 - margin) cam->position.y =  H/2 - margin;
    
    // Apply movement
    cam->position.x += forward_x * forward * speed + right_x * right * speed;
    cam->position.y += up * speed;
    cam->position.z += forward_z * forward * speed + right_z * right * speed;
}

static void event(MHMAPI_Context *ctx, const void *ev) {
    const ui8 *keyboard = mhmapi_input(ctx)->keyboard;
    MHMAPI_InputState *in = mhmapi_input(ctx);
    
    // Escape to quit
    if (keyboard[KC_ESCAPE]) {
        mhmapi_request_quit(ctx);
        return;
    }
    
    // Mouse look
    static int last_mx = 400, last_my = 300;
    int dx = in->mouse_x - last_mx;
    int dy = in->mouse_y - last_my;
    
    if (in->mouse_left) {  // Only rotate when left mouse held
        float sensitivity = 0.006f;
        ctx->cam.yaw += dx * sensitivity;
        ctx->cam.pitch -= dy * sensitivity;
        
        // Clamp pitch
        if (ctx->cam.pitch > 1.5f) ctx->cam.pitch = 1.5f;
        if (ctx->cam.pitch < -1.5f) ctx->cam.pitch = -1.5f;
    }
    
    last_mx = in->mouse_x;
    last_my = in->mouse_y;
    
    // Camera movement (faster with shift)
    float speed = keyboard[KC_RSHIFT] ? 8.0f : 3.0f;
    move_camera_input(&ctx->cam, keyboard, speed);
}

static void logger(MHMAPI_Context *ctx, const char *msg) {
    (void)ctx;
    printf("[LOG] %s\n", msg);
}

int main(void) {
    MHMAPI_Callbacks cb = { update, render, event };
    MHMAPI_Context *ctx = mhmapi_create(&cb, NULL);
    
    ctx->cam = camera_init((Vec3){0, 0, 0}, W, H, 75.0f); 
    //quad_tex = mhmapi_tex_load_png("assets/brick.png");
    mhmapi_set_logger(ctx, logger);
    
    ctx->win_desc = &(MHMAPI_WindowDesc){
        .title = "3D Room Demo - WASD + Mouse to move, Hold Left Click to look",
        .width = W,
        .height = H,
        .resizable = MHMAPI_TRUE
    };
    
    mhmapi_window_create(ctx);
    
    // Initialize scene
    init_scene(); 
    mhmapi_run(ctx);
    
    // Cleanup
    mhmapi_destroy(ctx);
    
    return 0;
}
