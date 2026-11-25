# Design Review — Pre-API Roadblocks

Analysis of the engine's architecture before building the public API, what
would have become a roadblock for game development, and how each was
resolved. Resolved items link the symptom to the fix; the "still open" items
are conscious trade-offs, not oversights.

## Resolved roadblocks

### 1. The engine was a single executable, not a library

**Symptom.** `CMakeLists.txt` produced one `obZcene` binary containing the
viewer, the CLI and the renderer in one target. A game could not link the
engine without forking it.

**Fix.** The engine is now a static library, `libobzcene` (Core, Renderer,
Texture, Viewer, Engine), with public headers under `include/obzcene`. The
`obZcene` executable is a thin CLI shell over it, and tests link the library
instead of recompiling engine sources per suite.

### 2. Scene objects were values in a vector — no stable identity

**Symptom.** `Scene::objects` was `std::vector<SceneObject>` and callers used
raw indices. Any insertion or removal shifted indices, so a game could not
hold "the player object" across frames — the first thing every game does.

**Fix.** `Scene::objects` is now `std::vector<ObjectHandle>`
(`shared_ptr<SceneObject>`). `add_object` returns a stable handle that stays
valid across any scene edit; `remove_object(handle)` and `index_of(handle)`
replace index juggling. The viewer's selection is a handle, not an index.

### 3. Transforms were yaw + uniform scale only

**Symptom.** `SceneObject` stored a single yaw float and a scalar scale.
Anything needing pitch, roll or non-uniform scale (a tipped-over lamp, a
stretched platform) was unrepresentable — and the scene file format baked
that limitation in.

**Fix.** `SceneObject` now carries full TRS: euler `rotation` (pitch/yaw/roll
in radians) and per-axis `scale`. `normal_matrix()` is a true
inverse-transpose, so non-uniform scale shades correctly. The `.obzscene`
format writes v2 lines (9 transform floats) and still reads v1 lines (2
floats: yaw + uniform scale), so old scenes keep loading.

### 4. The frame loop was hard-wired inside the editor

**Symptom.** `Viewer::run()` owned input polling, camera movement, light
editing and rendering. A game either forked that loop or had no loop at all.

**Fix.** `obzcene::Engine` (Engine/Engine.hpp) owns the runtime — window,
framebuffer, scene, camera, input, assets, audio — and drives
`obzcene::Game` callbacks: `on_init`, `on_update(dt)`, `on_render`,
`on_shutdown`. The editor executable is now just a front-end built on the
same library.

### 5. No reusable input layer

**Symptom.** The viewer polled `SDL_GetKeyboardState` inline with per-key
`was_down` flags scattered through the loop; there was no way to ask
"was this pressed this frame?" cleanly, and no mouse-delta plumbing for
games.

**Fix.** `obzcene::Input` (Engine/Input.hpp): a per-frame snapshot with
`down` / `pressed` / `released` edge detection and accumulated mouse delta,
keyed by a small engine `Key` enum (no SDL types leak into game code).

### 6. No audio at all

**Symptom.** Asset management stopped at models and textures; any game
beyond a silent demo was impossible.

**Fix.** `obzcene::Audio` (Engine/Audio.hpp) is a software mixer over an SDL
audio device: `load_audio_clip` decodes WAV to engine-native PCM (float32
stereo 48 kHz), `play()` returns a `SoundHandle`, voices mix in the audio
callback. When no device exists (headless CI), everything is a safe no-op.
`Assets` caches clips by name next to the model cache.

### 7. Render-loop duplication

**Symptom.** The per-object render loop with frustum culling and stats
merging existed twice (viewer and snapshot) after the multi-object change —
a third copy would have appeared in the engine.

**Fix.** `obzcene::render_scene()` (Viewer/Scene.hpp) is the single
implementation; both call sites use it.

## Still open (conscious trade-offs)

- **WAV-only audio.** Decoding goes through SDL_LoadWAV to avoid a codec
  dependency; OGG/MP3 would need SDL_mixer or stb_vorbis. The clip/mixer
  boundary means a new decoder slots in without touching the API.
- **No physics/animation systems.** The API is designed so these live in
  game code (object handles + `on_update(dt)`) or in future engine modules.
- **The editor viewer keeps its own input loop.** It predates `Input`; it
  works and is editor-local, so it stays until the editor is rebuilt on
  `Engine`.
- **No PBR/IBL.** The Blinn-Phong shading model is fixed; extending it is a
  shader-level change in `Shader.hpp`.
