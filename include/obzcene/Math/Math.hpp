#pragma once

/**
 * @file Math.hpp
 * @brief Aggregate header that exposes the entire math module in one include.
 *
 * Adding @c \#include <obzcene/Math/Math.hpp> brings in every math sub-header:
 * scalars (#math::Common.hpp), vectors, matrices, quaternions, interpolation,
 * and geometry helpers (AABB, Plane, Ray, Frustum).
 */
#include "obzcene/Math/AABB.hpp"
#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Geometry.hpp"
#include "obzcene/Math/Interpolation.hpp"
#include "obzcene/Math/Mat3.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Math/Plane.hpp"
#include "obzcene/Math/Quaternion.hpp"
#include "obzcene/Math/Ray.hpp"
#include "obzcene/Math/Vec2.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Math/Vec4.hpp"