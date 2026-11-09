/**
 * @file obZcene.cpp
 * @brief Program entry point: delegates everything to the app layer.
 *
 * The whole CLI/model/scene/rendering logic lives in obzcene::app
 * (App/App.hpp + App/Scene.hpp); this TU is intentionally a two-line
 * trampoline so the executable has a single obvious entry.
 *
 * @see App/App.hpp (obzcene::app::entry)
 */

#include <obzcene/App/App.hpp>

/**
 * @brief Start the software rasterizer.
 * @return Exit code forwarded from obzcene::app::entry.
 */
auto main(int argc, char** argv) -> int { return obzcene::app::entry(argc, argv); }