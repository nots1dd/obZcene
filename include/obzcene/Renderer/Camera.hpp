#pragma once

/**
 * @file Camera.hpp
 * @brief FPS-style camera with perspective/orthographic projection.
 *
 * The camera is the single point of truth for the view transform used by every
 * render path: `view_projection()` feeds the rasterizer (obzcene::rasterize,
 * per-vertex clip positions), `frustum()` derives the 6 view planes for
 * culling, and both `screen_to_ray`/`screen_to_ray_pixel` unproject pixels for
 * picking. It is
 * also what transforms per-frame light positions through
 * obzcene::resolve_lights (light attachment like the headlamp lives in
 * obzcene::Viewer).
 *
 * Aliases: #obzcene::CameraF, #obzcene::CameraD.
 *
 * @see src/Viewer/Viewer.cpp (movement keys/mouse drive this camera),
 *      src/App/Scene.cpp (setup_camera frames it around a model)
 */

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Geometry.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Math/Ray.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Math/Vec4.hpp"

#include <cmath>
#include <type_traits>

namespace obzcene
{

/** @brief Which projection the camera uses; drives projection_matrix(). */
enum class CameraProjection
{
  Perspective,
  Orthographic
};

/**
 * @brief Yaw/pitch camera whose basis is derived from yaw and pitch.
 * @tparam T Floating-point scalar (``float``/``double``).
 *
 * Movement methods translate along the camera's own forward/right/up axes,
 * matching the WASD + mouse control loop in obzcene::Viewer. Pitch is clamped
 * to ±89° to avoid gimbal flips.
 */
template <typename T> class Camera
{
  static_assert(std::is_floating_point_v<T>, "Camera requires a floating-point type");

public:
  /** @brief Default camera at the origin looking down -Z. */
  constexpr Camera() = default;

  /** @brief World-space eye position. */
  [[nodiscard]] constexpr auto position() const -> math::Vec3<T> { return m_position; }
  /** @brief Current yaw (radians). */
  [[nodiscard]] constexpr auto yaw() const -> T { return m_yaw; }
  /** @brief Current pitch (radians, clamped). */
  [[nodiscard]] constexpr auto pitch() const -> T { return m_pitch; }

  /**
   * @brief Unit forward vector of the camera.
   * @note Matches the -Z convention of math::Mat4::look_at; used by
   *       obzcene::Light::resolve_light for camera-attached lights.
   */
  [[nodiscard]] auto forward() const -> math::Vec3<T>
  {
    const auto cp = std::cos(m_pitch);
    return math::Vec3<T>{-std::sin(m_yaw) * cp, std::sin(m_pitch), -std::cos(m_yaw) * cp};
  }

  /** @brief Unit right vector (perpendicular to forward, in the world XZ plane). */
  [[nodiscard]] auto right() const -> math::Vec3<T>
  {
    return math::Vec3<T>{std::cos(m_yaw), T(0), -std::sin(m_yaw)};
  }

  /** @brief Unit up vector (perpendicular to forward and right). */
  [[nodiscard]] auto up() const -> math::Vec3<T>
  {
    const auto sp = std::sin(m_pitch);
    return math::Vec3<T>{std::sin(m_yaw) * sp, std::cos(m_pitch), std::cos(m_yaw) * sp};
  }

  /** @brief Move the eye to an absolute world position. */
  auto set_position(const math::Vec3<T>& pos) -> void { m_position = pos; }

  /**
   * @brief Set orientation from yaw and pitch angles (radians).
   * @note Pitch is clamped to ±89° internally (see #max_pitch).
   */
  auto set_orientation(T yaw, T pitch) -> void
  {
    m_yaw   = yaw;
    m_pitch = clamp_pitch(pitch);
  }

  /**
   * @brief Point the camera at `target` from `eye` and position it there.
   * @note No-op when `eye` and `target` coincide; used by obzcene::Viewer
   *       (reset pose) and src/App/Scene.cpp::setup_camera.
   */
  auto look_at(const math::Vec3<T>& eye, const math::Vec3<T>& target) -> void
  {
    auto dir = target - eye;
    if (dir.length_sq() < math::default_epsilon<T>())
      return;
    dir        = dir.normalized();
    m_position = eye;
    m_pitch    = clamp_pitch(std::asin(math::clamp(dir.y, T(-1), T(1))));
    m_yaw      = std::atan2(-dir.x, -dir.z);
  }

  /** @name Movement (relative to the camera's own axes) */
  /** @{ */
  auto move_forward(T amount) -> void { m_position += forward() * amount; }
  auto move_backward(T amount) -> void { m_position -= forward() * amount; }
  auto move_right(T amount) -> void { m_position += right() * amount; }
  auto move_left(T amount) -> void { m_position -= right() * amount; }
  auto move_up(T amount) -> void { m_position += up() * amount; }
  auto move_down(T amount) -> void { m_position -= up() * amount; }
  /** @} */

  /** @brief Rotate the view yaw by `angle` radians (mouse look). */
  auto yaw_by(T angle) -> void { m_yaw += angle; }

  /** @brief Rotate the view pitch by `angle` radians (clamped). */
  auto pitch_by(T angle) -> void { m_pitch = clamp_pitch(m_pitch + angle); }

  /** @brief Switch between perspective and orthographic projection. */
  auto set_projection(CameraProjection projection) -> void { m_projection = projection; }
  /** @brief Currently selected projection mode. */
  [[nodiscard]] constexpr auto projection() const -> CameraProjection { return m_projection; }

  /** @brief Update the aspect ratio (width/height) for the projection. */
  auto set_aspect(T aspect) -> void { m_aspect = aspect; }

  /**
   * @brief Configure a perspective projection.
   * @param fov_y  Vertical field of view in radians.
   * @param aspect Width/height.
   * @param near,far Clip distances (far must be > near).
   */
  auto set_perspective(T fov_y, T aspect, T near, T far) -> void
  {
    m_projection = CameraProjection::Perspective;
    m_fov_y      = fov_y;
    m_aspect     = aspect;
    m_near       = near;
    m_far        = far;
  }

  /**
   * @brief Configure an orthographic projection.
   * @param left,right,bottom,top  View volume in x/y.
   * @param near,far               Clip distances.
   */
  auto set_orthographic(T left, T right, T bottom, T top, T near, T far) -> void
  {
    m_projection   = CameraProjection::Orthographic;
    m_ortho_left   = left;
    m_ortho_right  = right;
    m_ortho_bottom = bottom;
    m_ortho_top    = top;
    m_near         = near;
    m_far          = far;
  }

  /** @brief Vertical field of view (radians); only used in perspective mode. */
  [[nodiscard]] constexpr auto fov() const -> T { return m_fov_y; }
  /** @brief Aspect ratio (width/height). */
  [[nodiscard]] constexpr auto aspect() const -> T { return m_aspect; }
  /** @brief Near clip distance. */
  [[nodiscard]] constexpr auto near_clip() const -> T { return m_near; }
  /** @brief Far clip distance. */
  [[nodiscard]] constexpr auto far_clip() const -> T { return m_far; }
  /** @brief Orthographic left bound. */
  [[nodiscard]] constexpr auto ortho_left() const -> T { return m_ortho_left; }
  /** @brief Orthographic right bound. */
  [[nodiscard]] constexpr auto ortho_right() const -> T { return m_ortho_right; }
  /** @brief Orthographic bottom bound. */
  [[nodiscard]] constexpr auto ortho_bottom() const -> T { return m_ortho_bottom; }
  /** @brief Orthographic top bound. */
  [[nodiscard]] constexpr auto ortho_top() const -> T { return m_ortho_top; }

  /** @brief World-to-view matrix (math::Mat4::look_at from the eye). */
  [[nodiscard]] auto view_matrix() const -> math::Mat4<T>
  {
    return math::Mat4<T>::look_at(m_position, m_position + forward(), world_up());
  }

  /** @brief Projection matrix for the current mode (perspective or ortho). */
  [[nodiscard]] auto projection_matrix() const -> math::Mat4<T>
  {
    if (m_projection == CameraProjection::Orthographic)
      return math::Mat4<T>::ortho(m_ortho_left, m_ortho_right, m_ortho_bottom, m_ortho_top, m_near,
                                  m_far);
    return math::Mat4<T>::perspective(m_fov_y, m_aspect, m_near, m_far);
  }

  /**
   * @brief Combined world-to-clip transform (`projection * view`).
   * @note This is the matrix obzcene::world_to_clip and obzcene::rasterize
   *       apply to every vertex.
   */
  [[nodiscard]] auto view_projection() const -> math::Mat4<T>
  {
    return projection_matrix() * view_matrix();
  }

  /** @brief Inverse of view_projection (clip-to-world, for unprojection). */
  [[nodiscard]] auto inv_view_projection() const -> math::Mat4<T>
  {
    return view_projection().inverse();
  }

  /**
   * @brief The six frustum planes extracted from the clip matrix.
   * @note Plane normals point inward; see #extract_plane.
   * @see math::Frustum for the containment tests consuming this.
   */
  [[nodiscard]] auto frustum() const -> math::Frustum<T>
  {
    const auto m      = view_projection();
    const auto left   = extract_plane(m, 0, 3, false);
    const auto right  = extract_plane(m, 0, 3, true);
    const auto bottom = extract_plane(m, 1, 3, false);
    const auto top    = extract_plane(m, 1, 3, true);
    const auto near   = extract_plane(m, 2, 3, false);
    const auto far    = extract_plane(m, 2, 3, true);
    return {left, right, bottom, top, near, far};
  }

  /**
   * @brief Unproject an NDC point into a world-space ray.
   * @param ndc_x,ndc_y  Normalized device coordinates, both in [-1,1],
   *                     y growing up.
   * @return A ray; in perspective mode the origin is the eye and the direction
   *         crosses `ndc`; in orthographic mode the origin is on the near
   *         plane and the direction is camera forward.
   * @see #screen_to_ray_pixel for the pixel-space convenience wrapper.
   */
  [[nodiscard]] auto screen_to_ray(T ndc_x, T ndc_y) const -> math::Ray<T>
  {
    if (m_projection == CameraProjection::Orthographic)
    {
      const auto x = m_ortho_left + (ndc_x + T(1)) * T(0.5) * (m_ortho_right - m_ortho_left);
      const auto y = m_ortho_bottom + (ndc_y + T(1)) * T(0.5) * (m_ortho_top - m_ortho_bottom);
      return math::Ray<T>{m_position + right() * x + up() * y + forward() * m_near, forward()};
    }
    const auto tan_half_v = std::tan(m_fov_y * T(0.5));
    const auto tan_half_h = tan_half_v * m_aspect;
    const auto dir =
      (forward() + right() * (ndc_x * tan_half_h) + up() * (ndc_y * tan_half_v)).normalized();
    return math::Ray<T>{m_position, dir};
  }

  /**
   * @brief Convert a top-left-origin pixel coordinate into a world-space ray.
   * @param px,py  Pixel coordinates (y grows downward, as in the render target).
   * @param width,height  Render target size in pixels.
   * @return Unprojected ray; see #screen_to_ray.
   */
  [[nodiscard]] auto screen_to_ray_pixel(T px, T py, T width, T height) const -> math::Ray<T>
  {
    const auto ndc_x = T(2) * (px + T(0.5)) / width - T(1);
    const auto ndc_y = T(1) - T(2) * (py + T(0.5)) / height;
    return screen_to_ray(ndc_x, ndc_y);
  }

private:
  /** @brief Fixed world up direction (+Y). */
  [[nodiscard]] static constexpr auto world_up() -> math::Vec3<T>
  {
    return math::Vec3<T>{T(0), T(1), T(0)};
  }

  /** @brief Maximum allowed absolute pitch (89°, keeps the basis stable). */
  [[nodiscard]] static constexpr auto max_pitch() -> T { return math::deg_to_rad(T(89)); }

  /** @brief Clamp a pitch angle into [-max_pitch, max_pitch]. */
  [[nodiscard]] static constexpr auto clamp_pitch(T value) -> T
  {
    return math::clamp(value, -max_pitch(), max_pitch());
  }

  /**
   * @brief Extract one frustum plane from a clip matrix (Gribb/Hartmann).
   * @param m    Projection*view clip matrix.
   * @param i,j  Row indices: the plane combines row `i` with fixed row `j`.
   * @param subtract  `true` for the +W sides (right/top/far), `false` for -W.
   * @return A normalized plane; used by #frustum.
   */
  [[nodiscard]] static auto extract_plane(const math::Mat4<T>& m, int i, int j, bool subtract)
    -> math::Plane<T>
  {
    const auto sign = subtract ? T(-1) : T(1);
    const auto a    = m(j, 0) + sign * m(i, 0);
    const auto b    = m(j, 1) + sign * m(i, 1);
    const auto c    = m(j, 2) + sign * m(i, 2);
    const auto d    = m(j, 3) + sign * m(i, 3);
    const auto len  = std::sqrt(a * a + b * b + c * c);
    return math::Plane<T>{math::Vec3<T>{a / len, b / len, c / len}, -d / len};
  }

  math::Vec3<T>    m_position{};
  T                m_yaw{0};
  T                m_pitch{0};
  T                m_fov_y{math::deg_to_rad(T(60))};
  T                m_aspect{T(16) / T(9)};
  T                m_near{T(0.1)};
  T                m_far{T(1000)};
  T                m_ortho_left{T(-10)};
  T                m_ortho_right{T(10)};
  T                m_ortho_bottom{T(-10)};
  T                m_ortho_top{T(10)};
  CameraProjection m_projection{CameraProjection::Perspective};
};

/** @brief Camera alias using `float`. */
using CameraF = Camera<float>;
/** @brief Camera alias using `double`. */
using CameraD = Camera<double>;

} // namespace obzcene