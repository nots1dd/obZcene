#pragma once

/**
 * @file obzcene.hpp
 * @brief Umbrella header for game developers: the whole public engine API.
 *
 * `#include <obzcene/obzcene.hpp>` gives a game everything it needs: the
 * engine facade and game hooks, input, assets, audio, the scene model, and
 * the math library. See docs/api.md for a guided tour.
 */

// Core
#include "obzcene/Core/Logger.hpp"
#include "obzcene/Core/Types.hpp"

// Engine facade
#include "obzcene/Engine/Assets.hpp"
#include "obzcene/Engine/Audio.hpp"
#include "obzcene/Engine/Engine.hpp"
#include "obzcene/Engine/Input.hpp"

// Math
#include "obzcene/Math/Math.hpp"

// Scene model
#include "obzcene/Viewer/Picking.hpp"
#include "obzcene/Viewer/Scene.hpp"
#include "obzcene/Viewer/SceneFile.hpp"
#include "obzcene/Viewer/SceneObject.hpp"

// Renderer types (for games that want custom shading control)
#include "obzcene/Renderer/Batch.hpp"
#include "obzcene/Renderer/Camera.hpp"
#include "obzcene/Renderer/Draw2D.hpp"
#include "obzcene/Renderer/GpuRenderer.hpp"
#include "obzcene/Renderer/Light.hpp"
