#pragma once

/**
 * @file Light.hpp
 * @brief Light types, attenuation and the shading pipeline used by the
 *        fragment shader.
 *
 * The `obzcene::Light<T>` struct is authored in scene code and resolved per
 * frame by `resolve_lights` (camera-space / "attached" lights keep their
 * offsets relative to the camera, so a light can follow it). `shade_point` is
 * the analytical lighting model the renderer's software path evaluates: a
 * lambert diffuse + Blinn-Phong specular, distance attenuation, and spot cones.
 * `shade_all` sums every light plus the ambient term.
 *
 * @see Renderer/Shader.hpp (shade_fragment drives shading with these),
 *      Renderer/Camera.hpp (basis used by resolve_light),
 *      Core/Types.hpp (Material supplies diffuse/specular/shininess)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Camera.hpp"

#include <cmath>
#include <type_traits>
#include <vector>

namespace obzcene
{

/** @brief Physical behaviour of a light: parallel rays, point, or cone. */
enum class LightType
{
  /** @brief Infinite-direction light: only `direction` matters, no position/attenuation. */
  Directional,
  /** @brief Omni-directional light with distance attenuation (position based). */
  Point,
  /** @brief Point light whose illumination is restricted to a cone (position + direction). */
  Spot
};

/**
 * @brief Coordinate space a light is authored in.
 *
 * `WorldSpace` keeps the light fixed in the scene; `CameraSpace` re-anchors
 * the light to the camera every frame (position and direction are treated as
 * offsets along the camera's right/up/forward axes), i.e. the "headlamp"
 * lights in obzcene::Viewer.
 *
 * @see #resolve_light, #resolve_lights
 */
enum class LightBinding
{
  WorldSpace,
  CameraSpace
};

/**
 * @brief A single light source.
 * @tparam T Floating-point scalar (``float``/``double``).
 *
 * `direction` stores the travel direction (the light shines the opposite way);
 * the default points straight down. Factory functions (#directional, #point,
 * #spot) normalize directions and set a sensible default colour/intensity.
 */
template <typename T> struct Light
{
  static_assert(std::is_floating_point_v<T>, "Light requires a floating-point type");

  /** @brief Kind of light, selects which fields take effect. */
  LightType type{LightType::Point};
  /** @brief Emitted colour; multiplied into every shade term. */
  math::Vec3<T> color{T(1), T(1), T(1)};
  /** @brief Strength scaling; combined with attenuation and spot factor. */
  T intensity{T(1)};
  /** @brief Position; used by Point/Spot lights and camera-space binding. */
  math::Vec3<T> position{};
  /** @brief Travel direction for Directional/Spot; default points down (-Y). */
  math::Vec3<T> direction{T(0), T(-1), T(0)};
  /** @brief Constant attenuation term (a/r²+d model: constant + linear*d + quadratic*d²). */
  T constant{T(1)};
  /** @brief Linear attenuation term. */
  T linear{T(0)};
  /** @brief Quadratic attenuation term. */
  T quadratic{T(0)};
  /** @brief Spot cone inner half-angle (radians), full intensity within it. */
  T spot_cutoff{math::deg_to_rad(T(45))};
  /** @brief Width of the soft falloff band beyond spot_cutoff (radians). */
  T spot_smooth{math::deg_to_rad(T(15))};
  /** @brief Authoring space; see obzcene::LightBinding. */
  LightBinding binding{LightBinding::WorldSpace};

  /**
   * @brief Create a directional light.
   * @param direction  Travel direction; normalized internally.
   */
  static auto directional(const math::Vec3<T>& direction,
                          const math::Vec3<T>& color     = math::Vec3<T>{T(1), T(1), T(1)},
                          T                    intensity = T(1)) -> Light<T>
  {
    Light<T> light;
    light.type      = LightType::Directional;
    light.direction = direction.normalized();
    light.color     = color;
    light.intensity = intensity;
    return light;
  }

  /**
   * @brief Create a point light at `position`.
   */
  static auto point(const math::Vec3<T>& position,
                    const math::Vec3<T>& color     = math::Vec3<T>{T(1), T(1), T(1)},
                    T                    intensity = T(1)) -> Light<T>
  {
    Light<T> light;
    light.type      = LightType::Point;
    light.position  = position;
    light.color     = color;
    light.intensity = intensity;
    return light;
  }

  /**
   * @brief Create a spot light: a point light restricted to a cone.
   * @param position  Apex of the cone.
   * @param direction  Cone axis (travel direction); normalized internally.
   */
  static auto spot(const math::Vec3<T>& position, const math::Vec3<T>& direction,
                   const math::Vec3<T>& color = math::Vec3<T>{T(1), T(1), T(1)}, T intensity = T(1))
    -> Light<T>
  {
    Light<T> light;
    light.type      = LightType::Spot;
    light.position  = position;
    light.direction = direction.normalized();
    light.color     = color;
    light.intensity = intensity;
    return light;
  }
};

/** @brief Light alias using `float`. */
using LightF = Light<float>;
/** @brief Light alias using `double`. */
using LightD = Light<double>;

/**
 * @brief Lambert diffuse factor: `max(n · l, 0)`.
 * @param normal     Surface normal (normalized).
 * @param light_dir  Unit vector from the surface toward the light.
 */
template <typename T>
[[nodiscard]] constexpr auto lambert(const math::Vec3<T>& normal, const math::Vec3<T>& light_dir)
  -> T
{
  return math::max(normal.dot(light_dir), T(0));
}

namespace detail
{

/**
 * @brief Fast `base^exponent` by binary exponentiation for non-negative
 *        integer exponents (used by blinn_phong when shininess is integral).
 */
template <typename T> [[nodiscard]] constexpr auto integer_power(T base, int exponent) -> T
{
  auto result = T(1);
  auto b      = base;
  while (exponent > 0)
  {
    if ((exponent & 1) != 0)
    {
      result *= b;
    }
    b *= b;
    exponent >>= 1;
  }
  return result;
}

} // namespace detail

/**
 * @brief Blinn-Phong specular term: `(n · h)^shininess` with `h` the
 *        normalized half-vector.
 * @param normal,light_dir,view_dir  Unit vectors.
 * @param shininess  Exponent; integral values ≤ 255 use fast integer power,
 *                   everything else falls back to `std::pow`.
 */
template <typename T>
[[nodiscard]] auto blinn_phong(const math::Vec3<T>& normal, const math::Vec3<T>& light_dir,
                               const math::Vec3<T>& view_dir, T shininess) -> T
{
  const auto half = (light_dir + view_dir).normalized();
  const auto ndh  = math::max(normal.dot(half), T(0));
  if (ndh <= T(0))
    return T(0);
  if (shininess == std::floor(shininess) && shininess >= T(0) && shininess <= T(255))
  {
    return detail::integer_power(ndh, static_cast<int>(shininess));
  }
  return std::pow(ndh, shininess);
}

/**
 * @brief Distance attenuation `1 / (c + l*d + q*d²)`.
 * @param constant,linear,quadratic  The three attenuation coefficients of a
 *        Light (default values give no falloff).
 */
template <typename T>
[[nodiscard]] constexpr auto attenuation(T distance, T constant, T linear, T quadratic) -> T
{
  return T(1) / (constant + linear * distance + quadratic * distance * distance);
}

/**
 * @brief Unit vector from surface point `point` toward a light.
 * @note Directional lights ignore position and use `-direction` (the light
 *       "travels" along `direction`, so it shines in the opposite way);
 *       Type::Point/Spot point at #light.position. Falling back to a
 *       zero-vector when `point` coincides with the light position.
 */
template <typename T>
[[nodiscard]] auto to_light(const Light<T>& light, const math::Vec3<T>& point) -> math::Vec3<T>
{
  if (light.type == LightType::Directional)
    return -light.direction;
  const auto dir = light.position - point;
  if (dir.length_sq() < math::default_epsilon<T>())
    return math::Vec3<T>{};
  return dir.normalized();
}

/** @brief Distance from `point` to a light (0 for directional lights). */
template <typename T>
[[nodiscard]] constexpr auto distance_to_light(const Light<T>& light, const math::Vec3<T>& point)
  -> T
{
  if (light.type == LightType::Directional)
    return T(0);
  return math::length(light.position - point);
}

/**
 * @brief Spot cone attenuation for a unit vector from the light toward the
 *        surface.
 * @return 1 inside `spot_cutoff`, 0 outside `spot_cutoff + spot_smooth`, and
 *         a linear falloff between the two (ignored unless `type == Spot`).
 */
template <typename T>
[[nodiscard]] constexpr auto spot_cone(const Light<T>& light, const math::Vec3<T>& to_surface_unit)
  -> T
{
  const auto cosine    = light.direction.dot(to_surface_unit);
  const auto cos_inner = std::cos(light.spot_cutoff);
  const auto cos_outer = std::cos(light.spot_cutoff + light.spot_smooth);
  if (cosine >= cos_inner)
    return T(1);
  if (cosine <= cos_outer || cos_inner <= cos_outer + math::default_epsilon<T>())
    return T(0);
  return (cosine - cos_outer) / (cos_inner - cos_outer);
}

/**
 * @brief Combined spot factor for a surface point: 1 for non-spot lights,
 *        otherwise #spot_cone toward the point (1 if the point coincides with
 *        the light apex).
 */
template <typename T>
[[nodiscard]] auto spot_factor(const Light<T>& light, const math::Vec3<T>& point) -> T
{
  if (light.type != LightType::Spot)
    return T(1);
  const auto to_surface = point - light.position;
  if (to_surface.length_sq() < math::default_epsilon<T>())
    return T(1);
  return spot_cone(light, to_surface.normalized());
}

/**
 * @brief Shade one surface point from one light.
 * @return `color * intensity * factor * (diffuse*n·l + specular*specterm)`
 *         where `factor` is distance attenuation (× spot factor for spots) and
 *         the specular term is Blinn-Phong. Returns black when the surface
 *         faces away (`n·l <= 0`) or the point coincides with a point/spot
 *         source; a zero `specular` skips the specular pass entirely.
 */
template <typename T>
[[nodiscard]] auto shade_point(const Light<T>& light, const math::Vec3<T>& point,
                               const math::Vec3<T>& normal, const math::Vec3<T>& view_dir,
                               const math::Vec3<T>& diffuse, const math::Vec3<T>& specular,
                               T shininess) -> math::Vec3<T>
{
  if (light.type == LightType::Directional)
  {
    const auto light_dir = -light.direction;
    const auto n_dot_l   = math::max(normal.dot(light_dir), T(0));
    if (n_dot_l <= T(0))
      return math::Vec3<T>{};
    auto spec = T(0);
    if (specular != math::Vec3<T>{})
      spec = blinn_phong(normal, light_dir, view_dir, shininess);
    return light.color * light.intensity * (diffuse * n_dot_l + specular * spec);
  }

  const auto to_source   = light.position - point;
  const auto distance_sq = to_source.length_sq();
  if (distance_sq < math::default_epsilon<T>())
    return math::Vec3<T>{};
  const auto n_dot_l_raw = normal.dot(to_source);
  if (n_dot_l_raw <= T(0))
    return math::Vec3<T>{};
  const auto distance     = std::sqrt(distance_sq);
  const auto inv_distance = T(1) / distance;
  const auto light_dir    = to_source * inv_distance;
  const auto n_dot_l      = n_dot_l_raw * inv_distance;
  const auto atten        = attenuation(distance, light.constant, light.linear, light.quadratic);
  const auto factor =
    atten * (light.type == LightType::Spot ? spot_cone(light, -to_source * inv_distance) : T(1));
  auto spec = T(0);
  if (specular != math::Vec3<T>{})
    spec = blinn_phong(normal, light_dir, view_dir, shininess);
  return light.color * light.intensity * factor * (diffuse * n_dot_l + specular * spec);
}

/**
 * @brief Cumulative lighting for a point: ambient base plus every light.
 * @see Renderer/Shader.hpp (fragment shading calls this with the resolved
 *      light list).
 */
template <typename T>
[[nodiscard]] auto shade_all(const math::Vec3<T>& point, const math::Vec3<T>& normal,
                             const math::Vec3<T>& view_dir, const math::Vec3<T>& diffuse,
                             const math::Vec3<T>& specular, T shininess,
                             const math::Vec3<T>& ambient, const std::vector<Light<T>>& lights)
  -> math::Vec3<T>
{
  auto result = ambient;
  for (const auto& light : lights)
    result += shade_point(light, point, normal, view_dir, diffuse, specular, shininess);
  return result;
}

/**
 * @brief Bring one light into world space for a frame.
 * @note CameraSpace lights are re-anchored to the camera: `position`/`direction`
 *       are offsets along the camera's right/up/forward axes, and the result
 *       is stamped WorldSpace so it is not transformed twice.
 * @see obzcene::Viewer (headlamp) and src/Viewer/Viewer.cpp.
 */
template <typename T>
[[nodiscard]] auto resolve_light(const Light<T>& light, const Camera<T>& camera) -> Light<T>
{
  if (light.binding != LightBinding::CameraSpace)
    return light;
  auto result    = light;
  result.binding = LightBinding::WorldSpace;
  const auto local_direction =
    result.type != LightType::Point
      ? math::normalized(camera.forward() * light.direction.z + camera.right() * light.direction.x +
                         camera.up() * light.direction.y)
      : math::Vec3<T>{};
  result.position = camera.position() + camera.right() * light.position.x +
                    camera.up() * light.position.y + camera.forward() * light.position.z;
  if (result.type != LightType::Point)
  {
    result.direction = local_direction;
  }
  return result;
}

/**
 * @brief Resolve a whole light list against a camera, preserving order.
 * @see #resolve_light, obzcene::shade_fragment (consumes the result).
 */
template <typename T>
[[nodiscard]] auto resolve_lights(const std::vector<Light<T>>& lights, const Camera<T>& camera)
  -> std::vector<Light<T>>
{
  auto result = std::vector<Light<T>>{};
  result.reserve(lights.size());
  for (const auto& light : lights)
  {
    result.push_back(resolve_light(light, camera));
  }
  return result;
}

/**
 * @brief Resolve a whole light list into caller-owned storage.
 * @param out  Target vector; cleared, then filled in order. Its capacity is
 *             retained across calls, so the per-frame path (obzcene::Viewer)
 *             does not allocate once it has grown to the scene's light count.
 * @see #resolve_lights (allocating convenience wrapper).
 */
template <typename T>
inline auto resolve_lights_into(const std::vector<Light<T>>& lights, const Camera<T>& camera,
                                std::vector<Light<T>>& out) -> void
{
  out.clear();
  if (out.capacity() < lights.size())
  {
    out.reserve(lights.size());
  }
  for (const auto& light : lights)
  {
    out.push_back(resolve_light(light, camera));
  }
}

} // namespace obzcene