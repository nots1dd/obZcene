# obZcene Documentation

obZcene is a CPU-based software 3D renderer written in C++23. It loads a
Wavefront OBJ model (with MTL materials and textures), rasterizes triangles
into an in-memory framebuffer with per-pixel lighting, and either shows the
result in an interactive SDL2 window or writes a single frame to a PPM image.

There is no GPU involvement: vertex transform, clipping, rasterization, depth
testing, and shading all run on the CPU in plain C++.

## Documents

| Document | Contents |
|---|---|
| [architecture.md](architecture.md) | Engine structure, module responsibilities, frame data flow |
| [build-and-usage.md](build-and-usage.md) | Dependencies, build steps, CLI options, viewer controls |
| [testing.md](testing.md) | Test layout, how to run, what is covered |
| [improvements.md](improvements.md) | Observed issues: code quality, performance, testing gaps |

## Capabilities at a glance

- Wavefront OBJ geometry loading (`v`/`vt`/`vn`/`f`/`o`/`g`), n-gon fan
  triangulation, computed face normals when absent
- MTL material parsing (`Ka`/`Kd`/`Ks`/`Ke`/`Ns`/`d`/`Tr`/`illum`/`map_*`)
- Diffuse texture mapping (any format SDL_image decodes), bilinear or nearest
  sampling, repeat/clamp/mirror wrapping
- Blinn-Phong shading with directional, point, and spot lights, distance
  attenuation, spot cones, and camera-attached ("headlamp") lights
- FPS-style camera (perspective and orthographic), mouse look, WASDQE +
  PgUp/PgDn movement
- Interactive scene editor: file browser to add objects (F4), grab/place,
  rotate/scale, delete, and `.obzscene` save/load (F5/F9)
- Software rasterizer with near/far clip-space triangle clipping, plus
  degenerate, backface, and viewport culling; multithreaded vertex transform
  and banded scan conversion; SSE2-accelerated inner loop where available
- Wireframe mode (Bresenham outlines)
- Headless snapshot mode writing binary PPM
- On-screen stats overlay (bitmap font drawn straight into the framebuffer)
- Header-only math library (vectors, matrices, quaternions, rays, planes,
  AABBs, frusta, interpolation)

## Repository layout

```
include/obzcene/   Public headers — most of the engine is header-only
  App/             Entry point + scene assembly declarations
  Core/            Logger, shared types, checked casts
  File/            OBJ and MTL parsers (header-only)
  Math/            Vec/Mat/Quaternion/Ray/Plane/AABB/Frustum (header-only)
  Renderer/        Camera, FrameBuffer, Mesh, Rasterizer, Shader, Light, Window
  Texture/         Texture storage/sampling, image load/write
  Viewer/          Interactive viewer, HUD overlay
  utils/           Timer and string helpers (used by Backtrace)
  Backtrace.hpp    Debug-only stack-trace event log
src/               .cpp implementations for the non-header-only parts
tests/             Catch2 test suites
assets/models/     Sample OBJ models (Cube, plane, Skull, bugatti, ...)
external/          Vendored dependencies (spdlog, CLI11, Catch2, libbacktrace)
```
