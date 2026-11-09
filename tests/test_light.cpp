#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "obzcene/Core/Types.hpp"
#include "obzcene/Math/Common.hpp"
#include "obzcene/Math/Vec3.hpp"
#include "obzcene/Renderer/Light.hpp"

#include <vector>

using Catch::Approx;
using obzcene::math::Vec3f;
using obzcene::attenuation;
using obzcene::blinn_phong;
using obzcene::distance_to_light;
using obzcene::lambert;
using obzcene::Light;
using obzcene::LightBinding;
using obzcene::LightF;
using obzcene::LightType;
using obzcene::resolve_light;
using obzcene::shade_all;
using obzcene::shade_point;
using obzcene::spot_factor;
using obzcene::to_light;

static constexpr auto sq2 = 0.7071067811865476f;

TEST_CASE("lambert term is the clamped normal dot light", "[light]")
{
  const auto normal = Vec3f{0, 1, 0};
  CHECK(lambert(normal, Vec3f{0, 1, 0}) == Approx(1.0f));
  CHECK(lambert(normal, Vec3f{0, 1, 1}.normalized()) == Approx(sq2));
  CHECK(lambert(normal, Vec3f{0, 0, 1}) == Approx(0.0f));
  CHECK(lambert(normal, Vec3f{0, -1, 0}) == Approx(0.0f));
}

TEST_CASE("blinn-phong specular peaks when half vector aligns with normal", "[light]")
{
  const auto normal    = Vec3f{0, 1, 0};
  const auto light_dir = Vec3f{0, 1, 0};
  const auto view_dir  = Vec3f{0, 0, 1};

  CHECK(blinn_phong(normal, light_dir, view_dir, 0.0f) == Approx(1.0f));
  CHECK(blinn_phong(normal, light_dir, view_dir, 2.0f) == Approx(0.5f));

  const auto grazing = Vec3f{1, 0, 0};
  CHECK(blinn_phong(normal, grazing, view_dir, 8.0f) == Approx(0.0f));
}

TEST_CASE("blinn-phong is specular-selective and shininess driven", "[light]")
{
  const auto normal    = Vec3f{0, 1, 0};
  const auto light_dir = Vec3f{0, 1, 0};
  const auto view_dir  = Vec3f{0, 1, 0};
  CHECK(blinn_phong(normal, light_dir, view_dir, 100.0f) == Approx(1.0f));
  CHECK(blinn_phong(normal, Vec3f{0, sq2, sq2}, Vec3f{0, sq2, sq2}, 4.0f) ==
        Approx(std::pow(sq2, 4.0f)));
}

TEST_CASE("attenuation combines constant linear and quadratic terms", "[light]")
{
  CHECK(attenuation(10.0f, 1.0f, 0.0f, 0.0f) == Approx(1.0f));
  CHECK(attenuation(5.0f, 0.0f, 1.0f, 0.0f) == Approx(0.2f));
  CHECK(attenuation(2.0f, 0.0f, 0.0f, 1.0f) == Approx(0.25f));
  CHECK(attenuation(1.0f, 1.0f, 1.0f, 1.0f) == Approx(1.0f / 3.0f));
}

TEST_CASE("light factories set sensible defaults", "[light]")
{
  const auto dir = LightF::directional({0, -1, 0});
  CHECK(dir.type == LightType::Directional);
  CHECK(dir.direction.is_almost_equal(Vec3f{0, -1, 0}));
  CHECK(dir.intensity == Approx(1.0f));
  CHECK(dir.color.is_almost_equal(Vec3f{1, 1, 1}));

  const auto pt = LightF::point({3, 2, 1});
  CHECK(pt.type == LightType::Point);
  CHECK(pt.position.is_almost_equal(Vec3f{3, 2, 1}));
  CHECK(pt.constant == Approx(1.0f));
  CHECK(pt.linear == Approx(0.0f));
  CHECK(pt.quadratic == Approx(0.0f));

  const auto sp = LightF::spot({0, 0, 0}, {0, 1, 0}, {1, 0, 0});
  CHECK(sp.type == LightType::Spot);
  CHECK(sp.direction.is_almost_equal(Vec3f{0, 1, 0}));
  CHECK(sp.color.is_almost_equal(Vec3f{1, 0, 0}));
}

TEST_CASE("to_light gives the surface-to-source direction", "[light]")
{
  const auto dir = LightF::directional({0, -1, 0});
  CHECK(to_light(dir, Vec3f{5, 5, 5}).is_almost_equal(Vec3f{0, 1, 0}));

  const auto pt = LightF::point({0, 5, 0});
  CHECK(to_light(pt, Vec3f{0, 0, 0}).is_almost_equal(Vec3f{0, 1, 0}));
  CHECK(to_light(pt, Vec3f{0, 4, 0}).is_almost_equal(Vec3f{0, 1, 0}));
  CHECK(distance_to_light(pt, Vec3f{0, 0, 0}) == Approx(5.0f));
  CHECK(distance_to_light(dir, Vec3f{0, 0, 0}) == Approx(0.0f));
}

TEST_CASE("spot factor is smooth between inner and outer cones", "[light]")
{
  LightF spot;
  spot.type        = LightType::Spot;
  spot.position    = Vec3f{0, 0, 0};
  spot.direction   = Vec3f{0, 0, -1};
  spot.spot_cutoff = obzcene::math::deg_to_rad(45.0f);
  spot.spot_smooth = obzcene::math::deg_to_rad(15.0f);

  CHECK(spot_factor(spot, Vec3f{0, 0, -5}) == Approx(1.0f));

  const auto at_50 =
    Vec3f{std::sin(obzcene::math::deg_to_rad(50.0f)), 0, -std::cos(obzcene::math::deg_to_rad(50.0f))};
  CHECK(spot_factor(spot, at_50 * 5.0f) == Approx(0.689439f).margin(1e-3f));

  const auto at_70 =
    Vec3f{std::sin(obzcene::math::deg_to_rad(70.0f)), 0, -std::cos(obzcene::math::deg_to_rad(70.0f))};
  CHECK(spot_factor(spot, at_70 * 5.0f) == Approx(0.0f));
}

TEST_CASE("spot factor is a hard cut when smooth width is zero", "[light]")
{
  LightF spot;
  spot.type        = LightType::Spot;
  spot.position    = Vec3f{0, 0, 0};
  spot.direction   = Vec3f{0, 0, -1};
  spot.spot_cutoff = obzcene::math::deg_to_rad(45.0f);
  spot.spot_smooth = 0.0f;

  const auto inside = Vec3f{0, 0, -5};
  CHECK(spot_factor(spot, inside) == Approx(1.0f));

  const auto outside =
    Vec3f{std::sin(obzcene::math::deg_to_rad(60.0f)), 0, -std::cos(obzcene::math::deg_to_rad(60.0f))} * 5.0f;
  CHECK(spot_factor(spot, outside) == Approx(0.0f));
}

TEST_CASE("shade_point with a directional light follows the material", "[light]")
{
  const auto light  = LightF::directional({0, -1, 0});
  const auto normal = Vec3f{0, 1, 0};
  const auto view   = Vec3f{0, 0, 1};

  CHECK(shade_point(light, Vec3f{0, 0, 0}, normal, view, Vec3f{1}, Vec3f{0}, 1.0f)
          .is_almost_equal(Vec3f{1, 1, 1}));
  CHECK(shade_point(light, Vec3f{0, 0, 0}, normal, view, Vec3f{0.5f}, Vec3f{0}, 1.0f)
          .is_almost_equal(Vec3f{0.5f, 0.5f, 0.5f}));
}

TEST_CASE("shade_point respects light direction and surface normal", "[light]")
{
  const auto light = LightF::directional({0, -1, 0});
  const auto view  = Vec3f{0, 0, 1};

  const auto angled = Vec3f{0, 0.5f, 0.8660254f};
  CHECK(shade_point(light, Vec3f{0, 0, 0}, angled, view, Vec3f{1}, Vec3f{0}, 1.0f)
          .is_almost_equal(Vec3f{0.5f, 0.5f, 0.5f}));

  const auto upward = LightF::directional({0, 1, 0});
  CHECK(shade_point(upward, Vec3f{0, 0, 0}, Vec3f{0, 1, 0}, view, Vec3f{1}, Vec3f{0}, 1.0f)
          .is_almost_equal(Vec3f{}));
}

TEST_CASE("shade_point applies distance attenuation for point lights", "[light]")
{
  auto light        = LightF::point({0, 2, 0}, Vec3f{1, 0, 0}, 1.0f);
  light.linear      = 0.5f;
  const auto normal = Vec3f{0, 1, 0};
  const auto view   = Vec3f{0, 0, 1};
  const auto result = shade_point(light, Vec3f{0, 0, 0}, normal, view, Vec3f{1}, Vec3f{0}, 1.0f);
  CHECK(result.is_almost_equal(Vec3f{0.5f, 0, 0}));

  auto lowered        = light;
  lowered.color       = Vec3f{0, 0, 1};
  lowered.constant    = 0.0f;
  lowered.linear      = 0.0f;
  lowered.quadratic   = 1.0f / 4.0f;
  const auto distance = 2.0f;
  const auto expected = 1.0f / (lowered.quadratic * distance * distance);
  CHECK(shade_point(lowered, Vec3f{0, 0, 0}, normal, view, Vec3f{1}, Vec3f{0}, 1.0f)
          .is_almost_equal(Vec3f{0, 0, expected}));
}

TEST_CASE("shade_point includes specular from the half vector", "[light]")
{
  const auto light  = LightF::directional({0, -1, 0});
  const auto normal = Vec3f{0, 1, 0};
  const auto view   = Vec3f{0, 1, 0};

  const auto result = shade_point(light, Vec3f{0, 0, 0}, normal, view, Vec3f{0}, Vec3f{0.5f}, 4.0f);
  CHECK(result.is_almost_equal(Vec3f{0.5f, 0.5f, 0.5f}));
}

TEST_CASE("shade_point returns black outside a spot cone", "[light]")
{
  LightF spot;
  spot.type        = LightType::Spot;
  spot.position    = Vec3f{0, 0, 0};
  spot.direction   = Vec3f{0, 0, -1};
  spot.spot_cutoff = obzcene::math::deg_to_rad(45.0f);
  spot.spot_smooth = 0.0f;

  const auto point  = Vec3f{5, 0, -2};
  const auto normal = obzcene::math::normalized(Vec3f{0, 0, 0} - point);
  const auto view   = Vec3f{0, 0, 1};
  CHECK(normal.dot(to_light(spot, point)) == Approx(1.0f));
  CHECK(shade_point(spot, point, normal, view, Vec3f{1}, Vec3f{0}, 1.0f).is_almost_equal(Vec3f{}));

  const auto no_cone = LightF::point({0, 0, 0}, Vec3f{1, 1, 1}, 1.0f);
  CHECK(
    !shade_point(no_cone, point, normal, view, Vec3f{1}, Vec3f{0}, 1.0f).is_almost_equal(Vec3f{}));
}

TEST_CASE("shade_all sums ambient and every light", "[light]")
{
  const auto                normal = Vec3f{0, 1, 0};
  const auto                view   = Vec3f{0, 0, 1};
  const std::vector<LightF> lights{
    LightF::directional({0, -1, 0}, Vec3f{1, 0, 0}, 1.0f),
    LightF::directional({0, -1, 0}, Vec3f{0, 1, 0}, 1.0f),
  };

  const auto result =
    shade_all(Vec3f{0, 0, 0}, normal, view, Vec3f{1}, Vec3f{0}, 1.0f, Vec3f{0.1f}, lights);
  CHECK(result.x == Approx(1.1f));
  CHECK(result.y == Approx(1.1f));
  CHECK(result.z == Approx(0.1f));

  const std::vector<LightF> none;
  CHECK(shade_all(Vec3f{0, 0, 0}, normal, view, Vec3f{1}, Vec3f{0}, 1.0f, Vec3f{0.05f}, none)
          .is_almost_equal(Vec3f{0.05f}));
}

TEST_CASE("shade_point integrates with an OBJ material", "[light]")
{
  obzcene::Material material;
  material.diffuse   = Vec3f{0.8f, 0.4f, 0.2f};
  material.specular  = Vec3f{1.0f, 1.0f, 1.0f};
  material.shininess = 16.0f;

  const auto light  = LightF::directional({0, -1, 0});
  const auto normal = Vec3f{0, 1, 0};
  const auto view   = Vec3f{0, 0, 1};

  const auto result = shade_point(light, Vec3f{0, 0, 0}, normal, view, material.diffuse,
                                  material.specular, material.shininess);

  const auto spec = std::pow(sq2, material.shininess);
  CHECK(result.x == Approx(0.8f + spec));
  CHECK(result.y == Approx(0.4f + spec));
  CHECK(result.z == Approx(0.2f + spec));
}

TEST_CASE("resolve_light leaves world-space lights unchanged", "[light]")
{
  obzcene::CameraF camera;
  camera.set_position(Vec3f{10, 0, 0});

  const auto world = LightF::point({4, 2, 1}, Vec3f{1, 0, 0}, 1.0f);
  const auto bound = resolve_light(world, camera);

  CHECK(bound.binding == LightBinding::WorldSpace);
  CHECK(bound.position.is_almost_equal(world.position));
  CHECK(bound.direction.is_almost_equal(world.direction));
}

TEST_CASE("resolve_light maps camera-space point lights into world space", "[light]")
{
  obzcene::CameraF camera;
  camera.set_position(Vec3f{10, 4, 2});

  auto local       = LightF::point({3, -1, 2}, Vec3f{1, 0, 0}, 1.0f);
  local.binding    = LightBinding::CameraSpace;
  const auto world = resolve_light(local, camera);
  const auto expected_pos =
    camera.position() + camera.right() * 3.0f + camera.up() * -1.0f + camera.forward() * 2.0f;

  CHECK(world.binding == LightBinding::WorldSpace);
  CHECK(world.position.is_almost_equal(expected_pos));
}

TEST_CASE("resolve_light maps camera-space directional lights into world space", "[light]")
{
  obzcene::CameraF camera;
  camera.set_position(Vec3f{0, 0, 0});

  auto local       = LightF::directional({0, -1, 0});
  local.binding    = LightBinding::CameraSpace;
  const auto world = resolve_light(local, camera);
  const auto expected =
    obzcene::math::normalized(camera.forward() * 0.0f + camera.right() * 0.0f + camera.up() * -1.0f);

  CHECK(world.binding == LightBinding::WorldSpace);
  CHECK(world.direction.is_almost_equal(expected));
  CHECK(world.direction.y == Approx(-1.0f));
  CHECK(world.direction.x == Approx(0.0f));
}