# Build and Usage

## Dependencies

- CMake ≥ 3.16 and a C++23 compiler
- SDL2 and SDL2_image (found via `find_package(... CONFIG)`)
- Git submodules: spdlog, CLI11, Catch2, libbacktrace (under `external/`)

## Build

```sh
make init                 # git submodule update --init --recursive
make init-dep-backtrace   # build libbacktrace into external/libbacktrace/build/install
make buildx               # configure + build Release into build/
make buildx-dbg           # configure + build Debug into build-dbg/ (defines OBZCENE_DEBUG_BUILD)
```

The binaries land in `build/obZcene` (release) and `build-dbg/obZcene`
(debug). Other useful Makefile targets: `make test` / `make test-dbg`
(ctest), `make fmt`, `make fmt-check`, `make tidy`, `make clean`.

## Running

```sh
./build/obZcene assets/models/Cube/cube.obj
```

### Command-line options

| Option | Default | Effect |
|---|---|---|
| `model` (positional) | — | Path to a Wavefront `.obj` file (must exist) |
| `-s, --snapshot PATH` | off | Render one frame to a PPM file and exit (headless) |
| `-W, --width N` | 1280 | Framebuffer/window width |
| `-H, --height N` | 720 | Framebuffer/window height |
| `--fov DEG` | 60 | Vertical field of view |
| `-w, --wireframe` | off | Start in wireframe mode |
| `--no-textures` | off | Skip texture loading |
| `--headlamp` | off | Attach a directional light to the camera |
| `--ambient X` | 0.1 | Global ambient intensity (scales each material's `Ka`) |
| `--light-pos x y z` | — | Add a point light (a spot light if `--light-dir` is also given) |
| `--light-dir x y z` | — | Add a directional light (or aim for `--light-pos`) |
| `--light-color r g b` | 1 1 1 | Colour of the user light, components in 0..1 |
| `--light-intensity X` | 1.0 | Intensity of the user light |

Setting `--light-pos` or `--light-dir` replaces the default three-light rig
(sun + two point lights) entirely.

### Examples

```sh
# Interactive viewer
./build/obZcene assets/models/plane/11803_Airplane_v1_l1.obj

# Headless 800x600 snapshot
./build/obZcene assets/models/Cube/cube.obj -s out.ppm -W 800 -H 600

# Custom spot light, no default rig
./build/obZcene model.obj --light-pos 0 5 5 --light-dir 0 -1 -0.5 --light-intensity 2
```

## Viewer controls

| Input | Action |
|---|---|
| Mouse | Look (relative-capture mode) |
| `W` `A` `S` `D` | Move forward/left/back/right |
| `Q` / `E` | Move down / up |
| `R` | Reset camera to the initial framing |
| `F1` | Toggle wireframe |
| `F2` | Toggle the stats overlay |
| `F3` | Toggle the camera headlamp |
| `[` / `]` | Cycle the active light (skips directional lights) |
| Arrow keys | Move the active light in the camera plane |
| `PgUp` / `PgDn` | Move the active light up / down |

Movement speed scales with the model's bounding radius (computed at load
time), so small and large models both feel reasonable.

## Snapshot mode

`-s` sets `SDL_VIDEODRIVER=dummy`, initializes SDL video (so SDL_image works),
renders one frame with the same pipeline the viewer uses, and writes a binary
P6 PPM. The rasterizer statistics (triangles culled per stage, pixels
shaded/written, milliseconds) are logged at INFO level. The destination's
parent directory must exist.

## Material support scope

The MTL parser reads the full classic keyword set (`Ka`/`Kd`/`Ks`/`Ke`/`Tf`/
`Ns`/`Ni`/`d`/`Tr`/`illum`, `map_Kd`/`map_Ks`/`map_Ka`/`map_bump`), but the
shader currently uses only `Ka` (ambient), `Kd` (diffuse), `Ks` + `Ns`
(specular), `Ke` (emissive), and the diffuse map `map_Kd`. Opacity,
transmission, refraction, `illum`, and the specular/normal/ambient maps are
parsed and stored but do not affect rendering.
