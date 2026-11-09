# Testing

Tests use Catch2 (vendored in `external/catch2`) and are registered with ctest.
Because the engine is mostly header-only, most test binaries compile nothing
but the test file itself.

## Running

```sh
make test        # ctest --test-dir build --output-on-failure   (release)
make test-dbg    # ctest --test-dir build-dbg --output-on-failure (debug)
```

Or run an individual binary directly, e.g. `./build/math-tests`.

## Test binaries

Eight ctest executables, 14 test files, roughly 210 `TEST_CASE`s:

| Binary | Files | Covers |
|---|---|---|
| `math-tests` | `test_vec.cpp`, `test_mat3.cpp`, `test_mat4.cpp`, `test_quat.cpp`, `test_geom.cpp` | Vectors, matrices, quaternions, geometry helpers (barycentric, frustum, ray, triangle tests) |
| `parser-tests` | `test_logger.cpp`, `test_mtl_parser.cpp`, `test_obj_parser.cpp` | Logger setup, MTL parsing, OBJ parsing (uses `assets/` via `ASSETS_DIR`) |
| `camera-tests` | `test_camera.cpp` | Camera basis, projections, look-at, unprojection |
| `light-tests` | `test_light.cpp` | Attenuation, spot cones, Blinn-Phong, `shade_point`, camera-space light resolution |
| `renderer-tests` | `test_renderer.cpp` | `build_mesh`, texture sampling/wrapping, end-to-end quad rasterization with shading |
| `culling-tests` | `test_culling.cpp` | Rasterizer cull stages, perspective-correct interpolation helpers |
| `overlay-tests` | `test_overlay.cpp` | Bitmap font metrics, text drawing, the editor HUD |
| `scene-tests` | `test_scene.cpp` | SceneObject transforms/bounds, `.obzscene` round-trip and malformed-line tolerance, file-browser navigation/filtering, model cache instancing |

Asset-dependent tests read `ASSETS_DIR`; the parser-tests target also defines
it as a compile definition, and test code falls back to the relative path
`assets/` when the variable is unset.

## What is covered well

- The math library has the deepest coverage (over half of all test cases).
- The rasterizer is tested both through its helpers (`raster_detail`,
  perspective blend) and through full quad-rendering scenarios.
- Parsers are tested against real files in `assets/`.
- The editor's non-SDL state is covered by `scene-tests`: scene-object
  transforms, scene-file round-trip (including malformed lines), file-browser
  navigation, and model-cache instancing.

## Known gaps

- `Viewer`, `Window`, and the snapshot path (`render_snapshot`,
  `write_ppm`, `load_image`) have no automated tests — they need SDL or file
  output, so they are currently exercised only manually.
- No golden-image regression test: snapshot output is never compared against
  a reference image.
- No performance/benchmark harness, so rasterizer changes have no
  measurable baseline.
- No CI configuration in the repository (no `.github/workflows` or similar);
  tests only run where a developer runs them.
