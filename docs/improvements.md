# Improvement Notes

Findings from a read-through of the codebase, grouped by area. The code
quality and performance items below have been **resolved** (each entry notes
the fix); the testing items remain open work.

## Code quality — resolved

1. **Global namespaces → nested under `obzcene`.** The math library lived in
   a top-level `namespace math`, and the debug helpers used top-level `core`
   and `utils::...` — collision-prone for a library. All are now nested:
   `obzcene::math` (all of `include/obzcene/Math/`), `obzcene::core`
   (`Backtrace.hpp`), `obzcene::utils::timer`, `obzcene::utils::string::transform`.
   Test files were updated to match. The module map in `Core/Types.hpp` now
   describes the real structure.

2. **OBJ relative indices now work.** The parse loop previously only called
   `resolve_index()` for indices `> 0`, so negative (relative) indices passed
   through unresolved and caused an assert/OOB access in `build_mesh()`. All
   indices are now resolved and validated via `detail::resolve_and_validate()`
   in `ObjParser.hpp`; index `0` and out-of-range positions invalidate the
   face, and out-of-range texcoord/normal indices degrade to "absent".

3. **Parser no longer crashes on malformed input.** `parse_face_vertex()`
   used unguarded `std::stoi`, which threw on bad tokens. It now uses
   `std::from_chars` (non-throwing), returns `std::optional<ObjVertexRef>`,
   and malformed faces are skipped with a warning and counted separately in
   the parse summary.

4. **Stale documentation references fixed.** `ImageWriter.hpp` and
   `FrameBuffer.hpp` pointed at a nonexistent `obzcene::App::capture_snapshot`;
   they now reference `obzcene::app::render_snapshot`. `Window.hpp` now
   correctly attributes the headless path to `src/App/Scene.cpp`.

5. **Dead option field removed.** `RasterOptions::wireframe` was never read by
   `rasterize()` (wireframe is a separate function chosen by the caller). The
   field is gone; call sites use `RasterOptions{.backface_cull = ...}`.

6. **Makefile repaired.** `all-checks` no longer depends on the nonexistent
   `check` target. The help text names obZcene (not "inLimbo") and Catch2 (not
   GTest). `HDR_DIRS_REGEX` is defined so `make tidy`'s `--header-filter`
   works. The `ci/apt|pacman|rpm/packages.txt` files `install-deps` requires
   now exist.

7. **Include style unified.** `Backtrace.hpp` now includes
   `"obzcene/utils/Env-Vars.hpp"` etc. like every other header, instead of
   bare `"utils/..."` paths.

8. **Duplicated frame setup extracted.** `obzcene::render_frame()` in
   `Renderer/Rasterizer.hpp` now owns the clear colour, depth clear,
   wireframe/solid dispatch, and rasterize options; both `Viewer::run()` and
   the snapshot path call it. The colours are shared as
   `obzcene::kClearColor` / `obzcene::kWireframeColor`.

9. **Unused material fields documented.** `Material::transmission`, `ior`,
   `opacity`, `illum`, and the specular/normal/ambient map paths are parsed
   but not used by `shade_fragment()`; their doc comments in `Core/Types.hpp`
   now say so explicitly, so users know only `map_Kd` is sampled.

## Performance — resolved

Measured at 1280×720 on an 8-core machine, release build (numbers are the
typical of 5 runs; the scene's own stats confirm the rendered output is
unchanged — same triangles drawn, same pixels shaded):

| Scene | Before | After | Speedup |
|---|---|---|---|
| plane (162k tris) | ~21 ms | ~7 ms | ~3.0× |
| bugatti (1.47M tris) | ~77 ms | ~47 ms | ~1.6× |

The bugatti render is pixel-identical to the old build; the plane render
differs only at depth ties (~1% RMSE), within the tolerance the culling test
suite already allows between the optimized and reference rasterizers.

1. **Multithreaded rasterizer.** `rasterize()` now runs in two parallel
   phases over `std::thread` workers (`RasterOptions::thread_count`, default
   = hardware concurrency). Phase 1 transforms vertex chunks and then —
   separated by a `std::barrier` — classifies triangle chunks, emitting
   compact `RasterJob`s binned per framebuffer band. Phase 2 scan-converts
   each band's jobs on its own thread. Bands partition pixel rows disjointly
   (no colour/depth races) and jobs keep triangle order within a band, so
   results match a single-band run.

2. **Whole-mesh frustum culling.** `render_frame()` now tests
   `camera.frustum().contains_aabb(mesh.bounds)` and skips the rasterizer
   entirely (empty stats) when the model is outside the view volume.

3. **Near-plane pop-out fixed.** Straddling triangles are clipped in clip
   space (Sutherland-Hodgman against the near and far clip-w planes) and
   fan-triangulated instead of being culled wholesale. Clipped vertices get
   exactly interpolated attributes (clip space is linear in the edge
   parameter). Clipping is rare, so it runs in a short serial pass after the
   parallel classification; fully-inside triangles never touch it.

4. **Per-frame light allocation removed.** `resolve_lights_into()` in
   `Light.hpp` fills caller-owned storage; `Viewer` keeps a reusable
   `m_resolved_lights` vector, so the interactive loop no longer allocates a
   fresh light vector every frame.

5. **SSE2 inner loop.** `rasterize_triangle_scan()` evaluates coverage and
   depth four pixels per iteration with SSE2 (enabled when `__SSE2__` is
   defined), replicating the scalar math lane-for-lane so images are
   identical; shading runs scalar per surviving pixel, with a scalar tail
   and a full scalar fallback on non-SSE2 builds.

6. **Vertex cache in `build_mesh()`.** Corners are now deduplicated through
   an `unordered_map` keyed on the OBJ (position, texcoord, normal) index
   triple, so the vertex pool holds each unique corner once instead of three
   copies per triangle (cube: 36 → 24 vertices; bugatti: 4.4M → 1.7M). The
   transform phase shrinks accordingly.

7. **Bounded scratch buffers.** The vertex pool lives in a `thread_local`
   `PoolStorage` (malloc-backed, so growing skips the redundant zero-init
   memset a `std::vector` would pay per frame) and is `realloc`-shrunk when a
   much smaller mesh follows a large one; per-chunk job lists and band bins
   are cleared, not destroyed, so their capacity is reused across frames.

## Testing — open

1. **No CI.** There is no `.github/workflows` (or equivalent) — the Makefile
   has `verify-deps`/`fmt-check`/`tidy` targets and ctest wiring, but nothing
   runs them automatically. A minimal workflow (configure, build, `ctest`,
   `fmt-check`) would cover most regressions.

2. **Untested components.** `Window`, `Viewer`, `ImageWriter` (`write_ppm`),
   `ImageLoader` (`load_image`), the `App`/`Scene` helpers
   (`compute_scene_view`, `make_lights`, `load_scene`, `render_snapshot`),
   `Interpolation.hpp`, and the `Backtrace`/`utils` helpers have no tests.
   The SDL-bound pieces are understandably hard; the rest are plain functions
   and easy to cover. `write_ppm` in particular can be round-trip tested
   against a known buffer.

3. **No golden-image regression test.** Snapshot mode produces a PPM but
   nothing compares it to a reference, so visual regressions (lighting,
   texture flipping, culling changes) can only be caught by eye. (The
   performance work above was validated by hand this way — bugatti output
   was pixel-identical to the pre-change build.)

4. **No benchmark harness.** There is no way to measure rasterizer
   performance changes in-tree; the numbers above were produced with ad-hoc
   snapshot timing. A fixed-scene timing test (render N frames of a known
   model, report ms) would make regressions visible automatically.

5. **Inconsistent `ASSETS_DIR` plumbing.** `parser-tests` receives it as both
   a compile definition and an environment variable; `renderer-tests` only as
   an environment variable; the test code reads only the environment. It
   works, but one mechanism should be picked.
