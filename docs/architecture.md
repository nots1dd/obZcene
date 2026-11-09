# Architecture

## Overview

obZcene is a single executable. `src/obZcene.cpp` is a two-line `main` that
forwards to `obzcene::app::entry()` (`src/App/App.cpp`). Everything else hangs
off the modules below. Most of the engine is header-only; only the SDL-facing
code, logging, image writing, and the viewer/overlay live in `.cpp` files.

```
obZcene.cpp (main)
   │
   ▼
App ──► File parsers (OBJ/MTL) ──► Mesh builder ──► Texture loader
   │                                                        │
   ▼                                                        ▼
Scene assembly (camera framing, lights)            Renderer (rasterizer + shader)
   │                                                        │
   ├─ interactive mode ─► Viewer (SDL window, input, overlay HUD)
   └─ snapshot mode ────► one frame ─► ImageWriter (PPM)
```

## Module responsibilities

### App (`include/obzcene/App`, `src/App`)

The top layer. `entry()` parses the command line with CLI11 and picks one of
two run modes:

- **Interactive** (default): opens `obzcene::Viewer` with the loaded scene.
- **Snapshot** (`-s out.ppm`): forces SDL's dummy video driver, renders exactly
  one frame, writes it with `write_ppm`, and exits.

`Scene.cpp` contains the load-time helpers:

- `load_scene()` — builds the editor `Scene`: loads a `--scene` file when
  given (objects + lights + saved camera pose via `apply_scene_file`), then
  adds the positional `--model` OBJ on top through the model cache, and
  computes the camera framing over the combined world bounds.
- `compute_scene_view()` — derives an eye position, look-at center, bounding
  radius, and movement speed from the scene's world-space AABB so the scene
  is visible on frame one.
- `make_lights()` — either a default three-light rig (directional "sun" +
  two radius-scaled point lights) or the user-specified `--light-pos` /
  `--light-dir` overrides (only used when the scene file provided none).
- `render_snapshot()` — the headless single-frame path; renders every object
  with its model matrix into one framebuffer.

### Core (`include/obzcene/Core`, `src/Core`)

- `Types.hpp` — fixed-width aliases (`ui8`, `ui32`, `usz`, ...) plus the asset
  intermediate representation: `Material`, `ObjVertexRef`, `ObjFace`,
  `ObjObject`, `ObjData`. `ObjData` mirrors the flat arrays of an OBJ file and
  is what `build_mesh()` consumes.
- `Logger.hpp/.cpp` — thin static wrapper over spdlog. `LOG_INFO(...)` logs to
  the core logger; `LOG_MODULE_INFO("OBJ", ...)` uses lazily created named
  loggers. `Logger::init()` must run first (done in `entry()`).
- `Casts.hpp` — `narrow_cast<To>(value)`, an asserting integral cast used
  wherever sizes become indices.
- `Threading.hpp` — `threading::parallel_run(workers, fn)`, the engine's only
  thread-spawning site; rasterization phases call it instead of owning
  thread management themselves.

### File (`include/obzcene/File`)

Both parsers are header-only, line-oriented, and forgiving (unknown keywords
log a warning and are skipped).

- `ObjParser.hpp` — `parse_obj()` turns a `.obj` file into `ObjData`. Handles
  1-based and negative (relative) indices, `o`/`g` grouping, `usemtl`
  assignment, and pulls in the referenced `mtllib` via `parse_mtl()`. Faces
  with fewer than 3 corners are dropped; n-gons pass through for later
  triangulation.
- `MtlParser.hpp` — `parse_mtl()` produces one `Material` per `newmtl` block.

### Math (`include/obzcene/Math`)

Header-only, templated on scalar type (`float`/`double`), in namespace
`obzcene::math`. `Vec2/3/4`, `Mat3`, `Mat4` (look-at, perspective, ortho,
inverse), `Quaternion`,
`Ray`, `Plane`, `AABB`, `Frustum`, plus interpolation helpers (smoothstep,
Catmull-Rom, slerp/nlerp support) and geometry utilities (barycentric,
point-in-triangle, reflect/refract). `Math.hpp` is the umbrella include.
`Pack4.hpp` provides `math::Pack4f`, a 4-wide float pack with SSE2 and
scalar backends used by the rasterizer's inner loop (bit-identical results
either way).

This module has no dependencies on the rest of the engine and carries the
largest share of the tests.

### Renderer (`include/obzcene/Renderer`, `src/Renderer`)

The heart of the engine.

- `Camera.hpp` — yaw/pitch FPS camera. Produces the view, projection, and
  combined view-projection matrices (perspective or orthographic), extracts
  the six frustum planes, and unprojects pixels back to world rays
  (`screen_to_ray`). Pitch is clamped to ±89°.
- `Mesh.hpp` — the render-ready geometry: flat `MeshVertex` array
  (position/normal/texcoord), `MeshTriangle` indices + material index,
  material table, and a precomputed AABB. `build_mesh()` converts `ObjData`
  into this form: fan-triangulates n-gons, deduplicates corners through a
  vertex cache keyed on the OBJ (position, texcoord, normal) index triple,
  and computes a Newell face normal for faces lacking vertex normals.
- `Rasterizer.hpp` — the hot loop, `rasterize()`. It runs in two parallel
  phases over `std::thread` workers (`RasterOptions::thread_count`, default
  = hardware concurrency):
  1. **Setup** — vertex chunks are transformed once (world → clip → screen),
     then, separated by a `std::barrier`, triangle chunks are classified:
     near/far clip-w test, degenerate/backface/viewport culling, and emission
     of compact `RasterJob`s binned per framebuffer band. Triangles
     straddling the near/far planes are deferred to a short serial clip pass
     (Sutherland-Hodgman in clip space, fan-triangulated) instead of being
     dropped, so nothing pops out at the near plane.
  2. **Scan conversion** — each worker owns a horizontal band of pixel rows
     and rasterizes only the jobs binned into it (disjoint rows → no
     colour/depth races; triangle order preserved → results identical to a
     single-band run). The inner loop uses edge-function barycentrics
     4 pixels at a time via `math::Pack4f` (Math/Pack4.hpp — SSE2 backend
     when available, scalar otherwise; thread fan-out itself lives in
     `threading::parallel_run`, Core/Threading.hpp). Covered pixels
     get perspective-correct world/normal/uv interpolation, a depth test,
     and a `shade_fragment()` call. Returns `RenderStats` counters
     (triangles processed/culled by stage, pixels shaded/written) that feed
     the HUD. `rasterize_wireframe()` is a separate Bresenham outline path.
  Above it, `render_frame()` clears the target, applies whole-mesh frustum
  culling against `Mesh::bounds`, and dispatches wireframe/solid.
- `Shader.hpp` — the software equivalent of a fragment shader.
  `shade_fragment()` samples the material's diffuse texture (bilinear, repeat,
  v flipped), then accumulates lighting with `shade_all()` and adds the
  emissive term. `FragmentUniforms` bundles the frame-wide state (camera,
  resolved lights, texture table, ambient scale).
- `Light.hpp` — light types (directional/point/spot), factories, and the
  lighting math: Lambert diffuse, Blinn-Phong specular (with a fast integer
  power path for integral shininess), quadratic distance attenuation, and spot
  cone falloff. `LightBinding::CameraSpace` lights are re-anchored to the
  camera each frame by `resolve_lights()` — this is how the headlamp works.
- `FrameBuffer.hpp` — row-major colour (`Color` = 8-bit RGBA) + `float` depth
  buffers, flat-indexed accessors for the rasterizer fast path.
- `Window.hpp/.cpp` — SDL2 facade. Owns the window/renderer and a streaming
  texture; `present()` uploads the framebuffer. Also pumps events, tracks key
  state, and accumulates relative mouse deltas. Not used in snapshot mode.
- `RenderStats.hpp` — plain counter struct returned by `rasterize()`.

### Texture (`include/obzcene/Texture`, `src/Texture`)

- `Texture.hpp` — CPU image (interleaved bytes, 1/3/4 channels) plus sampling:
  nearest and bilinear, with repeat/clamp/mirror coordinate wrapping. Also
  procedural `make_checkerboard()` / `make_solid()` generators.
- `ImageLoader.hpp` — `load_image()` decodes via SDL_image into RGBA.
- `TextureLoader.cpp` — `load_material_textures()` builds the diffuse-texture
  table indexed parallel to `Mesh::materials` (missing maps stay empty and
  materials fall back to their flat `Kd` colour).
- `ImageWriter.cpp` — `write_ppm()` serializes a framebuffer as binary P6 PPM.

### Viewer (`include/obzcene/Viewer`, `src/Viewer`)

- `Scene` — the editor's document model: a flat list of `SceneObject`s
  (shared `Model` + position/yaw/scale transform) plus the light list.
  Models are cached by canonical path (`Scene::load_model`), so adding the
  same OBJ twice instances it instead of reloading. `world_bounds()` gives
  the transform-aware combined AABB used for camera framing.
- `SceneObject.hpp` — one placed instance: shared model pointer + TRS
  transform. `model_matrix()` = translate · rotate_y · scale, applied
  per-vertex by the rasterizer (see ModelTransform in Rasterizer.hpp);
  `transformed_bounds()` maps the mesh AABB to world space.
- `SceneFile.hpp` — header-only `.obzscene` persistence: a line-oriented
  text format (`object` / `light` / `camera`) parsed with `std::from_chars`;
  malformed lines are skipped with a warning. `apply_scene_file` /
  `save_scene` (src/Viewer/Scene.cpp) bridge it to `Scene`.
- `FileBrowser.hpp` — a UI-free directory-listing picker state machine:
  current directory, sorted entries (directories first, `.obj` filter),
  cursor, descend/pick/parent navigation. The overlay renders it, the viewer
  drives it.
- `Viewer` — the interactive loop and editor. Owns the `Window`, `CameraF`,
  `FrameBuffer` and the `Scene`. Each frame: compute `dt`, dispatch input by
  mode (fly camera / grab-and-place object / file browser), resize the
  framebuffer if the window changed, clear, resolve lights, rasterize every
  object with its model matrix, draw the HUD + browser overlay, present.
  Frame time is smoothed with an exponential moving average.
- `Overlay` — a fixed 5×7 bitmap font (ASCII 0x20–0x7E) drawn directly into
  the framebuffer, plus editor panels: `draw_hud()` (scene stats, selection,
  light, key legend), `draw_status_bar()` (mode/selection/messages) and
  `draw_browser()`.

### Backtrace + utils

`Backtrace.hpp` is a debug-build-only (`OBZCENE_DEBUG_BUILD`) in-memory event
log with libbacktrace stack captures; both macros compile to no-ops in release.
`obzcene::utils::timer` (timestamp string) and `obzcene::utils::string::transform` (lowercase)
exist to support it. `utils/Env-Vars.hpp` defines the `OBZCENE_STACK_TRACE`
environment variable name.

## Frame data flow (solid interactive frame)

1. **Input** — `Window::handle_events()`, then mode dispatch: fly camera,
   grab (object placement), or the file browser (`src/Viewer/Viewer.cpp`).
2. **Light resolution** — `resolve_lights_into(m_lights, m_camera,
   m_resolved_lights)` converts camera-space lights to world space into a
   reusable buffer (no per-frame allocation); headlamp appended if enabled.
3. **Clear** — colour to dark grey, depth to +∞ (`clear_frame`).
4. **Per object** — the viewer builds a `ModelTransform` (model + normal
   matrices) and the transform-aware world AABB per `SceneObject`, then calls
   `render_object`, which frustum-culls against those bounds and rasterizes:
   - **Vertex stage** — every vertex transformed by
     `view_projection × model` (parallel over vertex chunks); screen position,
     `1/w`, and NDC depth cached in a reused pool.
   - **Triangle culling** — near/far classification, degenerate, backface,
     viewport, in parallel over triangle chunks; survivors become
     `RasterJob`s binned per framebuffer band. Straddlers are clipped in clip
     space rather than dropped, and counted in `RenderStats`.
   - **Rasterization** — scanline fill with incremental barycentric weights,
     parallel over horizontal bands; SSE2 4-wide coverage/depth evaluation
     when available.
   - **Fragment stage** — perspective-correct attributes → depth test →
     `shade_fragment()` (texture × diffuse, then all lights, plus emissive) →
     8-bit colour write.
5. **Overlay** — HUD, status bar and (when open) the browser panel drawn into
   the same framebuffer.
6. **Present** — framebuffer uploaded to the SDL streaming texture.

Snapshot mode runs steps 3–4 once per object and writes the result with
`write_ppm()`.

## Key design decisions

- **Header-only core.** The math library, parsers, rasterizer, shader, and
  lighting are all inline headers. Compilation is slower but there is one
  obvious definition site per component, and the tests exercise the headers
  directly.
- **Data flow is pull-based and explicit.** `ObjData → Mesh → rasterize()`;
  textures and lights are plain vectors passed by const reference through
  `FragmentUniforms`. There is no scene graph or entity system — the editor
  scene is a flat list of `SceneObject`s (shared model + TRS transform), and
  the only cache is the path-keyed model cache in `Scene`.
- **Index alignment is the contract.** `Mesh::materials[i]`,
  `FragmentUniforms::diffuse_textures[i]`, and `MeshTriangle::material_index`
  all refer to the same index, guaranteed by building both from the same
  `ObjData`.
- **Parallelism without races.** The rasterizer splits work along two
  disjoint axes — vertex/triangle chunks in the setup phase and horizontal
  row bands in scan conversion — so no two threads write the same vertex or
  pixel, and per-frame output is identical to a single-threaded run.
  Triangles straddling the near/far clip planes are clipped
  (Sutherland-Hodgman) rather than culled, in a short serial pass after the
  parallel classification.
