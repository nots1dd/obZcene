#pragma once

/**
 * @file App.hpp
 * @brief Application entry point: CLI dispatch and headless snapshot mode.
 *
 * Implemented in src/App/App.cpp (:entry) and src/App/Scene.cpp (scene
 * helpers). It owns the two run modes of the program: the interactive viewer
 * (obzcene::Viewer) when launched with a model path, or a one-shot offline
 * render writing the frame to an image when `--out` is given. Both modes
 * funnel through obzcene::app::load_scene + render_snapshot / Viewer::run.
 *
 * @see src/obZcene.cpp (main calls this), Viewer/Viewer.hpp,
 *      App/Scene.hpp (scene/light/camera construction)
 */

namespace obzcene::app
{

/**
 * @brief Run the application.
 * @param argc,argv  Process arguments: the OBJ model path plus options
 *                   `-s/--snapshot`, `-W/--width`, `-H/--height`, `--fov`,
 *                   `-w/--wireframe`, `--no-textures`, `--headlamp`,
 *                   `--ambient`, and the light overrides `--light-pos`,
 *                   `--light-dir`, `--light-color`, `--light-intensity`
 *                   (see #entry in src/App/App.cpp).
 * @return Process exit code; 0 on success, non-zero on invalid arguments or
 *         failed scene loading/rendering.
 */
auto entry(int argc, char** argv) -> int;

} // namespace obzcene::app