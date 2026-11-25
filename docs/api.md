# Game API Guide

The public game API is one header, one facade class, and one hook class:

```cpp
#include <obzcene/obzcene.hpp>   // everything below

class MyGame : public obzcene::Game { /* on_init / on_update / ... */ };

int main()
{
  obzcene::Engine engine{{.title = "My Game", .width = 1280, .height = 720}};
  MyGame          game;
  return engine.run(game);
}
```

Link against `libobzcene` (CMake target `obzcene`); headers are under
`include/`. A complete worked example lives in `examples/sandbox.cpp`
(build target `sandbox-example`).

## The frame loop

`Engine::run(game)` runs until the window closes or `engine.quit()` is
called (`quit()` works inside `on_init` too — handy when required assets
fail to load). Each frame:

1. Input is pumped into `engine.input()`.
2. If the fly camera is enabled (default), WASD + QE/PgUp/PgDn + mouse move
   the camera. Disable it with `EngineConfig::fly_camera = false` to drive
   the camera yourself.
3. `game.on_update(engine, dt)` — your game logic.
4. The scene renders; `game.on_render(engine)` lets you draw on top via
   `engine.framebuffer()` (crosshairs, sky sprites, custom HUD).
5. Present.

Timing accessors: `engine.dt()` (frame delta, seconds), `engine.time()`
(total, seconds), `engine.fps()` (smoothed). `Esc` quits by default; `F1`
toggles wireframe, `F2` toggles the stats HUD.

## Scenes and objects

`engine.scene()` is the mutable `Scene`. Objects are `SceneObject`s: a shared
model plus a TRS transform (`position`, `rotation` euler radians XYZ,
per-axis `scale`).

```cpp
auto model  = engine.assets().model("skull");            // loads models/skull.obj
auto skull  = engine.scene().add_object(model, {0, 0, 0});
skull->rotation.y = 0.7F;
skull->scale      = {2, 2, 2};
```

`add_object` returns a stable `ObjectHandle` — keep it across frames and
mutate it in `on_update`; handles stay valid when other objects are added or
removed. `engine.scene().remove_object(handle)` deletes. Duplicate loads of
the same file are instanced automatically (shared geometry, independent
transforms).

Lights live in `engine.scene().lights`:

```cpp
engine.scene().lights.push_back(obzcene::LightF::directional({-1, -1, -1}, {1, 1, 1}, 1.0F));
engine.scene().lights.push_back(obzcene::LightF::point({0, 3, 0}, {0.6F, 0.7F, 1.0F}, 1.0F));
engine.scene().lights.push_back(obzcene::LightF::spot({0, 5, 0}, {0, -1, 0}, {1, 1, 1}, 1.5F));
```

## Performance: batching many instances

Every scene object pays a fixed render cost (frustum test, vertex setup,
worker fan-out), so thousands of small objects — voxel terrain, scattered
props — are dominated by overhead, not pixels. Bake instances of the same
mesh into one mesh up front (`Renderer/Batch.hpp`):

```cpp
std::vector<obzcene::math::Mat4f> transforms;
for (/* each instance */)
  transforms.push_back(obzcene::math::Mat4f::translation(x, y, z));

auto chunk       = std::make_shared<obzcene::Model>();
chunk->mesh      = obzcene::bake_instances(block->mesh, transforms);
chunk->textures  = block->textures;
engine.scene().add_object(std::move(chunk), {0, 0, 0});  // vertices are world-space
```

`bake_instances` shares one material table across all instances;
`append_mesh(dst, src, transform)` is the general accumulator for mixing
different source meshes. Baked meshes are static — to change one instance,
re-bake. Keep batches small enough that a re-bake is cheap (e.g. one terrain
chunk), and you get per-batch frustum culling for free. See
`examples/sandbox.cpp` for the full chunked-terrain pattern.


## Picking (raycasting)

`Viewer/Picking.hpp` finds what the crosshair or mouse is pointing at by
testing each object's transform-aware bounds:

```cpp
const auto ray = obzcene::math::Rayf{cam.position(), cam.forward()};
if (const auto hit = obzcene::raycast(scene, ray, 12.0F))        // nearest only
  // hit->object, hit->t, hit->point, hit->face (entry-face axis)
const auto all = obzcene::raycast_all(scene, ray, 12.0F,
    +[](const obzcene::ObjectHandle& o) { return o->name != "gun"; });
```

`raycast_all` sorts nearest-first and accepts a predicate — use it as a
broad-phase, then refine inside the hit objects (the sandbox drills into
chunk hits to find individual blocks and the face to place against).

## Distance fog

`FogSettings` fades fragments to a colour between two distances
(`end <= start` disables). It works identically on both renderer backends
and usually tracks the sky colour:

```cpp
engine.set_fog({{0.47F, 0.66F, 1.0F}, 24.0F, 48.0F});   // start, end in blocks
engine.set_fog(obzcene::FogSettings{});                  // off
```

## 2D overlays

`Renderer/Draw2D.hpp` draws HUD sprites into the framebuffer inside
`on_render` — backend-agnostic (GPU mode composites it as an alpha layer):

```cpp
auto& fb = engine.framebuffer();
obzcene::draw2d::rect(fb, 10, 10, 110, 30, {20, 20, 20, 160}, false, true); // blended
obzcene::draw2d::disc(fb, cx, cy, 12, {255, 240, 170});
obzcene::draw2d::square(fb, cx, cy, 36, {255, 240, 170}, /*sky_only=*/true);
obzcene::draw2d::line(fb, 0, 0, 100, 60, {255, 255, 255});
```

`sky_only` paints only untouched (infinite-depth) pixels, which is how the
sandbox embeds its sun/moon behind terrain.

## Assets

`engine.assets()` resolves short names against the asset roots (default
`assets/`, plus `"."`; add more with `assets().add_root(...)`).

```cpp
auto tree = engine.assets().model("models/tree.obj");  // or bare "tree"
auto shot = engine.assets().audio("laser");            // loads laser.wav
```

Both are cached by resolved path — repeated calls return the same shared
instance. `assets().exists(name)` probes without loading. Audio is WAV only
for now.

### Debugging the asset search path

The registry logs its active search path at construction:

```
[CORE] [info] Assets: search path is [assets; .]
[CORE] [info] Assets:   root[0] 'assets' (ok)
[CORE] [info] Assets:   root[1] '.' (ok)
```

A root that does not exist is flagged `MISSING`, so a typo in
`EngineConfig::assets_dir` is visible at startup. When a lookup fails, the
warning lists the roots that were searched:

```
[CORE] [warn] Model 'cube.obj' not found; searched roots: [assets; .]
```

Ways to change the search path:

- `EngineConfig::assets_dir` — the primary root (default `"assets"`).
- `engine.assets().add_root("more/models")` — append fallback directories.
- `OBZCENE_ASSETS_DIR=/path/to/assets` — environment override, searched
  first; no recompile needed.
- `engine.assets().roots()` / `describe_roots()` — inspect the active list
  programmatically (e.g. print it from your game's debug console).

## Audio

```cpp
const auto handle = engine.audio().play(shot, {.volume = 0.8F, .loop = false});
engine.audio().set_volume(handle, 0.5F);
engine.audio().stop(handle);
engine.audio().stop_all();
```

Playback is a no-op when no audio device exists (headless runs), so the same
game binary works in CI.

Positional audio: `obzcene::spatial_options(source, listener, right, volume)`
computes distance rolloff into `volume` and direction into `PlayOptions::pan`
(-1 left .. +1 right) — the sandbox plays block-break sounds at the block.

```cpp
const auto opts = obzcene::spatial_options(block_center, cam.position(), cam.right(), 0.7F, 3.0F);
engine.audio().play(thud, opts);
```

## Input

`engine.input()` is a per-frame snapshot:

```cpp
engine.input().down(obzcene::Key::W);        // held
engine.input().pressed(obzcene::Key::Space); // edge: this frame only
engine.input().released(obzcene::Key::Space);
engine.input().mouse_delta();                // relative motion this frame

// Mouse buttons (Left/Middle/Right), same edge semantics:
engine.input().down(obzcene::MouseButton::Left);
engine.input().pressed(obzcene::MouseButton::Left);
engine.input().released(obzcene::MouseButton::Left);
```

The `Key` enum covers the common game keys (letters, arrows, F-keys, Space,
Enter, Escape, Tab, brackets, etc. — see `Engine/Input.hpp`).

### Mouse look sensitivity

The built-in fly camera's look speed is configurable at startup and at
runtime:

```cpp
obzcene::EngineConfig config;
config.mouse_sensitivity = 2.0F;   // 1.0 is the default speed
obzcene::Engine engine{config};

engine.set_mouse_sensitivity(0.5F);          // change live (e.g. ADS)
engine.mouse_sensitivity();                  // read back
```

When `fly_camera` is off, `engine.input().mouse_delta()` hands you the raw
relative motion so you can apply your own sensitivity curve.

## Startup options

`EngineConfig` fields (all optional): `title`, `width`, `height`, `fov_y`,
`wireframe`, `headlamp`, `ambient`, `no_textures`, `show_hud`, `fly_camera`,
`assets_dir`, `scene_file` (a `.obzscene` to load at startup, camera pose
included).

Presentation knobs, settable at startup or at runtime:

```cpp
config.clear_color    = obzcene::Color{120, 168, 255};        // sky/background
config.texture_filter = obzcene::TextureFilter::Nearest;      // crisp pixel art

engine.set_clear_color({22, 22, 28});                    // change live
engine.set_texture_filter(obzcene::TextureFilter::Linear);   // smooth (default)

// Vertical sky gradient (zenith/horizon) instead of a flat colour:
engine.set_sky_gradient(true, {38, 105, 212}, {135, 190, 235});
```

## Renderer backends (CPU / GPU)

The scene renderer is switchable: the default **CPU** path is the software
rasterizer (deterministic, portable, no GPU needed); **GPU** renders the same
scene through OpenGL 3.3 with the same lighting model and conventions:

```cpp
obzcene::EngineConfig config;
config.backend = obzcene::RendererBackend::Gpu;   // default: Cpu
```

Both backends share the scene/camera/light APIs, per-object frustum culling
and wireframe toggle. In GPU mode the CPU framebuffer becomes a transparent
overlay layer composited over the 3D frame, so `on_render` overlays (custom
HUD, crosshairs, sky sprites) keep working unchanged. The sandbox example
ships a switch: `OBZCENE_GPU=1 ./build/sandbox-example`.

## Scene persistence

The same `.obzscene` files the editor saves (F5) load in a game:

```cpp
obzcene::EngineConfig config;
config.scene_file = "level1.obzscene";
```

or programmatically:

```cpp
obzcene::save_scene(engine.scene(), "level1.obzscene", engine.camera());
auto data = obzcene::load_scene_file("level1.obzscene");  // parse only
```

The format is plain text, forward/backward tolerant — see
docs/build-and-usage.md#scenes.
