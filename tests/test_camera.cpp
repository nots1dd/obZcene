#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Mat4.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Math/Vec4.hpp"
#include "obzcene/Renderer/Camera.hpp"

#include <cmath>
#include <utility>

using Catch::Approx;
using obzcene::math::Mat4f;
using obzcene::math::Vec3f;
using obzcene::math::Vec4f;
using obzcene::Camera;
using obzcene::CameraF;
using obzcene::CameraProjection;

static constexpr auto sq2 = 0.7071067811865476f;

TEST_CASE("default camera faces -Z from the origin", "[camera]")
{
  CameraF cam;
  CHECK(cam.position().is_almost_equal(Vec3f{0, 0, 0}));
  CHECK(cam.yaw() == Approx(0.0f));
  CHECK(cam.pitch() == Approx(0.0f));
  CHECK(cam.forward().is_almost_equal(Vec3f{0, 0, -1}));
  CHECK(cam.right().is_almost_equal(Vec3f{1, 0, 0}));
  CHECK(cam.up().is_almost_equal(Vec3f{0, 1, 0}));
  CHECK(cam.projection() == CameraProjection::Perspective);
  CHECK(cam.fov() == Approx(obzcene::math::deg_to_rad(60.0f)));
  CHECK(cam.aspect() == Approx(16.0f / 9.0f));
  CHECK(cam.near_clip() == Approx(0.1f));
  CHECK(cam.far_clip() == Approx(1000.0f));
}

TEST_CASE("camera basis is orthonormal at arbitrary orientation", "[camera]")
{
  const std::pair<float, float> angles[]{{-2.0f, 0.2f},
                                         {0.7f, -0.6f},
                                         {1.3f, 0.9f},
                                         {obzcene::math::deg_to_rad(75.0f), obzcene::math::deg_to_rad(-88.0f)}};
  CameraF                       cam;
  for (auto [yaw, pitch] : angles)
  {
    cam.set_orientation(yaw, pitch);
    const auto f = cam.forward();
    const auto r = cam.right();
    const auto u = cam.up();
    CHECK(r.dot(u) == Approx(0.0f).margin(1e-6f));
    CHECK(r.dot(f) == Approx(0.0f).margin(1e-6f));
    CHECK(u.dot(f) == Approx(0.0f).margin(1e-6f));
    CHECK(r.length() == Approx(1.0f));
    CHECK(u.length() == Approx(1.0f));
    CHECK(f.length() == Approx(1.0f));
  }
}

TEST_CASE("camera basis at canonical yaw and pitch", "[camera]")
{
  CameraF cam;

  cam.set_orientation(obzcene::math::deg_to_rad(90.0f), 0.0f);
  CHECK(cam.forward().is_almost_equal(Vec3f{-1, 0, 0}));
  CHECK(cam.right().is_almost_equal(Vec3f{0, 0, -1}));
  CHECK(cam.up().is_almost_equal(Vec3f{0, 1, 0}));

  cam.set_orientation(obzcene::math::deg_to_rad(180.0f), 0.0f);
  CHECK(cam.forward().is_almost_equal(Vec3f{0, 0, 1}));
  CHECK(cam.right().is_almost_equal(Vec3f{-1, 0, 0}));

  cam.set_orientation(0.0f, obzcene::math::deg_to_rad(45.0f));
  CHECK(cam.forward().is_almost_equal(Vec3f{0, sq2, -sq2}));
  CHECK(cam.right().is_almost_equal(Vec3f{1, 0, 0}));
  CHECK(cam.up().is_almost_equal(Vec3f{0, sq2, sq2}));

  cam.set_orientation(obzcene::math::deg_to_rad(90.0f), obzcene::math::deg_to_rad(30.0f));
  CHECK(cam.forward().is_almost_equal(Vec3f{-0.8660254f, 0.5f, 0}));
  CHECK(cam.up().is_almost_equal(Vec3f{0.5f, 0.8660254f, 0}));
}

TEST_CASE("pitch is clamped away from the poles", "[camera]")
{
  CameraF cam;
  cam.pitch_by(obzcene::math::deg_to_rad(200.0f));
  CHECK(cam.pitch() == Approx(obzcene::math::deg_to_rad(89.0f)));
  cam.pitch_by(-obzcene::math::deg_to_rad(400.0f));
  CHECK(cam.pitch() == Approx(-obzcene::math::deg_to_rad(89.0f)));

  cam.set_orientation(0.5f, obzcene::math::deg_to_rad(91.0f));
  CHECK(cam.pitch() == Approx(obzcene::math::deg_to_rad(89.0f)));
  cam.yaw_by(obzcene::math::deg_to_rad(25.0f));
  CHECK(cam.yaw() == Approx(0.5f + obzcene::math::deg_to_rad(25.0f)));
}

TEST_CASE("look_at orients the camera toward a target", "[camera]")
{
  CameraF cam;
  cam.look_at(Vec3f{0, 0, 0}, Vec3f{0, 0, -5});
  CHECK(cam.position().is_almost_equal(Vec3f{0, 0, 0}));
  CHECK(cam.forward().is_almost_equal(Vec3f{0, 0, -1}));
  CHECK(cam.pitch() == Approx(0.0f));
  CHECK(cam.yaw() == Approx(0.0f));

  cam.look_at(Vec3f{0, 0, 0}, Vec3f{0, 2, -5});
  const auto expected = Vec3f{0, 2, -5}.normalized();
  CHECK(cam.forward().is_almost_equal(expected));
  CHECK(cam.pitch() == Approx(std::asin(expected.y)));

  cam.look_at(Vec3f{1, 2, 3}, Vec3f{4, 2, 7});
  CHECK(cam.position().is_almost_equal(Vec3f{1, 2, 3}));
  CHECK(cam.forward().is_almost_equal(Vec3f{3, 0, 4}.normalized()));
}

TEST_CASE("look_at with coincident eye and target is a no-op", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{2, 3, 4});
  cam.set_orientation(0.4f, 0.2f);
  cam.look_at(Vec3f{2, 3, 4}, Vec3f{2, 3, 4});
  CHECK(cam.position().is_almost_equal(Vec3f{2, 3, 4}));
  CHECK(cam.yaw() == Approx(0.4f));
  CHECK(cam.pitch() == Approx(0.2f));
}

TEST_CASE("camera movement follows the view basis", "[camera]")
{
  CameraF cam;
  cam.move_forward(2.0f);
  CHECK(cam.position().is_almost_equal(Vec3f{0, 0, -2}));
  cam.move_backward(1.0f);
  cam.move_right(3.0f);
  cam.move_left(1.0f);
  cam.move_up(2.0f);
  cam.move_down(1.0f);
  CHECK(cam.position().is_almost_equal(Vec3f{2, 1, -1}));

  cam.set_orientation(obzcene::math::deg_to_rad(90.0f), 0.0f);
  cam.move_right(3.0f);
  cam.move_forward(3.0f);
  CHECK(cam.position().is_almost_equal(Vec3f{-1, 1, -4}));

  cam.set_orientation(0.0f, obzcene::math::deg_to_rad(45.0f));
  cam.move_up(2.0f);
  CHECK(cam.position().is_almost_equal(Vec3f{-1, 1 + 2.0f * sq2, -4 + 2.0f * sq2}));
}

TEST_CASE("camera at origin has an identity view matrix", "[camera]")
{
  CameraF cam;
  CHECK(cam.view_matrix().is_almost_equal(Mat4f::identity()));
}

TEST_CASE("camera view matrix matches look_at", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{1, 2, 3});
  cam.set_orientation(0.5f, -0.3f);
  const auto expected =
    Mat4f::look_at(cam.position(), cam.position() + cam.forward(), Vec3f{0, 1, 0});
  CHECK(cam.view_matrix().is_almost_equal(expected));
}

TEST_CASE("camera view maps points in front of the camera to negative z", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 5});
  const auto p = cam.view_matrix() * Vec4f{0, 0, 0, 1};
  CHECK(p.x == Approx(0.0f));
  CHECK(p.y == Approx(0.0f));
  CHECK(p.z == Approx(-5.0f));
  CHECK(p.w == Approx(1.0f));

  cam.set_position(Vec3f{1, 2, 3});
  const auto q = cam.view_matrix() * Vec4f{1, 2, -1, 1};
  CHECK(q.x == Approx(0.0f));
  CHECK(q.y == Approx(0.0f));
  CHECK(q.z == Approx(-4.0f));
}

TEST_CASE("perspective projection maps camera-space points to ndc", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 0});
  cam.set_perspective(obzcene::math::deg_to_rad(90.0f), 1.0f, 1.0f, 10.0f);

  const auto identity_view = Mat4f::perspective(obzcene::math::deg_to_rad(90.0f), 1.0f, 1.0f, 10.0f);
  CHECK(cam.view_projection().is_almost_equal(identity_view));

  const auto to_ndc = [&](const Vec3f& p) -> Vec3f
  {
    const auto c = cam.view_projection() * Vec4f{p.x, p.y, p.z, 1};
    return Vec3f{c.x / c.w, c.y / c.w, c.z / c.w};
  };

  CHECK(to_ndc({0, 0, -1}).z == Approx(-1.0f));
  CHECK(to_ndc({0, 0, -10}).z == Approx(1.0f));
  const auto center = to_ndc({0, 0, -5});
  CHECK(center.x == Approx(0.0f));
  CHECK(center.y == Approx(0.0f));
  CHECK(to_ndc({5, 0, -5}).x == Approx(1.0f));
  CHECK(to_ndc({0, 5, -5}).y == Approx(1.0f));
  CHECK(to_ndc({6, 0, -5}).x > 1.0f);
}

TEST_CASE("orthographic projection maps to ndc linearly", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 0});
  cam.set_orthographic(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 10.0f);
  CHECK(cam.projection() == CameraProjection::Orthographic);

  const auto c = cam.view_projection() * Vec4f{5, 3, -2, 1};
  CHECK(c.w == Approx(1.0f));
  CHECK(c.x == Approx(1.0f));
  CHECK(c.y == Approx(0.6f));
  CHECK(c.z == Approx(-7.0f / 9.0f));
}

TEST_CASE("inverse view projection unprojects points", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{3, -2, 7});
  cam.set_orientation(0.8f, 0.35f);
  cam.set_perspective(obzcene::math::deg_to_rad(60.0f), 1.5f, 0.1f, 100.0f);
  const auto vp  = cam.view_projection();
  const auto ivp = cam.inv_view_projection();
  for (auto p : {Vec3f{0, 0, 0}, Vec3f{1, 2, 3}, Vec3f{-5, 4, -9}})
  {
    const auto clip = vp * Vec4f{p.x, p.y, p.z, 1};
    const auto back = ivp * clip;
    CHECK(back.x / back.w == Approx(p.x).margin(1e-4f));
    CHECK(back.y / back.w == Approx(p.y).margin(1e-4f));
    CHECK(back.z / back.w == Approx(p.z).margin(1e-4f));
  }
}

TEST_CASE("frustum encloses the perspective volume", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 0});
  cam.set_perspective(obzcene::math::deg_to_rad(60.0f), 1.0f, 0.1f, 100.0f);

  const auto f = cam.frustum();
  for (const auto& plane : f.planes)
    CHECK(plane.normal.length() == Approx(1.0f));

  CHECK(f.contains_point({0, 0, -10}));
  CHECK(f.contains_point({2, 1, -10}));
  CHECK(f.contains_point({0, 0, -0.11f}));
  CHECK(f.contains_point({0, 0, -99.0f}));
  CHECK(!f.contains_point({0, 0, 5}));
  CHECK(!f.contains_point({0, 0, -0.05f}));
  CHECK(!f.contains_point({0, 0, -200.0f}));
  CHECK(!f.contains_point({30, 0, -40}));

  cam.set_orthographic(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 10.0f);
  const auto o = cam.frustum();
  CHECK(o.contains_point({1, 1, -5}));
  CHECK(!o.contains_point({6, 0, -5}));
  CHECK(!o.contains_point({0, 0, -15}));
}

TEST_CASE("screen to ray reaches the frustum edges", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 0});
  cam.set_perspective(obzcene::math::deg_to_rad(90.0f), 1.0f, 0.1f, 100.0f);

  const auto center = cam.screen_to_ray(0, 0);
  CHECK(center.origin.is_almost_equal(Vec3f{0, 0, 0}));
  CHECK(center.direction.is_almost_equal(Vec3f{0, 0, -1}));

  const auto top_left = cam.screen_to_ray(-1, 1);
  CHECK(top_left.direction.is_almost_equal(Vec3f{-1, 1, -1}.normalized()));
  const auto bottom_right = cam.screen_to_ray(1, -1);
  CHECK(bottom_right.direction.is_almost_equal(Vec3f{1, -1, -1}.normalized()));
}

TEST_CASE("pixel coordinates convert to ndc before ray cast", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 0});
  cam.set_perspective(obzcene::math::deg_to_rad(90.0f), 1.0f, 0.1f, 100.0f);

  const auto mid = cam.screen_to_ray_pixel(4, 4, 9, 9);
  CHECK(mid.direction.is_almost_equal(Vec3f{0, 0, -1}));

  const auto top_left = cam.screen_to_ray_pixel(0, 0, 9, 9);
  CHECK(top_left.direction.is_almost_equal(Vec3f{-8.0f / 9.0f, 8.0f / 9.0f, -1}.normalized()));
}

TEST_CASE("orthographic picking casts parallel rays", "[camera]")
{
  CameraF cam;
  cam.set_position(Vec3f{0, 0, 5});
  cam.set_orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);

  const auto center = cam.screen_to_ray(0, 0);
  CHECK(center.direction.is_almost_equal(Vec3f{0, 0, -1}));
  CHECK(center.origin.is_almost_equal(Vec3f{0, 0, 4}));

  const auto corner = cam.screen_to_ray(1, 1);
  CHECK(corner.direction.is_almost_equal(Vec3f{0, 0, -1}));
  CHECK(corner.origin.is_almost_equal(Vec3f{1, 1, 4}));
}

TEST_CASE("projection type can be toggled", "[camera]")
{
  CameraF cam;
  cam.set_projection(CameraProjection::Orthographic);
  CHECK(cam.projection() == CameraProjection::Orthographic);
  CHECK(cam.projection_matrix()(0, 0) == Approx(2.0f / 20.0f));

  cam.set_projection(CameraProjection::Perspective);
  CHECK(cam.projection() == CameraProjection::Perspective);
}

TEST_CASE("camera works in double precision", "[camera]")
{
  using obzcene::math::Mat4d;
  using obzcene::math::Vec3d;
  using obzcene::CameraD;

  CameraD cam;
  cam.look_at(Vec3d{0, 0, 0}, Vec3d{1, 1, 1}.normalized() * 5.0);
  const auto f = cam.forward();
  CHECK(f.is_almost_equal(Vec3d{1, 1, 1}.normalized()));
  CHECK(cam.view_matrix().is_almost_equal(
    Mat4d::look_at(cam.position(), cam.position() + f, Vec3d{0, 1, 0})));
}