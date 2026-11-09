#include <obzcene/Viewer/Overlay.hpp>

#include <obzcene/Math/Vec2.hpp>
#include <obzcene/Math/Vec3.hpp>
#include <obzcene/Renderer/Camera.hpp>
#include <obzcene/Renderer/Color.hpp>
#include <obzcene/Renderer/FrameBuffer.hpp>
#include <obzcene/Renderer/Light.hpp>
#include <obzcene/Renderer/Mesh.hpp>
#include <obzcene/Renderer/RenderStats.hpp>
#include <obzcene/Texture/Texture.hpp>
#include <obzcene/Viewer/Scene.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using obzcene::math::Vec2f;
using obzcene::math::Vec3f;

namespace
{

auto count_set(const obzcene::FrameBuffer& fb, const obzcene::Color& background) -> std::size_t
{
  auto count = std::size_t{0};
  for (auto y = 0; y < fb.height(); ++y)
  {
    for (auto x = 0; x < fb.width(); ++x)
    {
      const auto c = fb.at(x, y);
      if (c.r != background.r || c.g != background.g || c.b != background.b)
      {
        ++count;
      }
    }
  }
  return count;
}

auto sample_stats() -> obzcene::RenderStats
{
  obzcene::RenderStats stats;
  stats.triangles_processed       = 128;
  stats.triangles_rendered        = 96;
  stats.triangles_culled_backface = 20;
  stats.triangles_culled_viewport = 8;
  stats.triangles_culled_nearfar  = 2;
  stats.triangles_degenerate      = 2;
  stats.pixels_shaded             = 4096;
  stats.pixels_written            = 3456;
  return stats;
}

auto sample_mesh() -> obzcene::Mesh
{
  obzcene::Mesh mesh;
  mesh.bounds.min   = Vec3f{-1, -2, -3};
  mesh.bounds.max   = Vec3f{1, 2, 3};
  mesh.object_count = 3;
  mesh.materials.resize(2);
  mesh.materials[0].name            = "metal";
  mesh.materials[0].diffuse_texture = "metal.png";
  mesh.materials[1].name            = "plastic";
  mesh.vertices.resize(30);
  mesh.triangles.resize(10);
  return mesh;
}

auto sample_textures() -> std::vector<obzcene::Texture>
{
  auto textures = std::vector<obzcene::Texture>(2);
  textures[0].resize(64, 32, 3);
  textures[1].resize(16, 16, 4);
  return textures;
}

auto sample_lights() -> std::vector<obzcene::LightF>
{
  return std::vector<obzcene::LightF>{
    obzcene::LightF::directional(Vec3f{0, 0, -1}),
    obzcene::LightF::point(Vec3f{0, 2, 0}),
    obzcene::LightF::spot(Vec3f{0, 0, 1}, Vec3f{0, 0, -1}),
  };
}

} // namespace

TEST_CASE("text width scales with content length", "[overlay][text]")
{
  REQUIRE(obzcene::overlay::text_width("") == 0);
  REQUIRE(obzcene::overlay::text_width("A") == 6);
  REQUIRE(obzcene::overlay::text_width("AB") == 12);
  REQUIRE(obzcene::overlay::text_width("FPS 60") == 36);
  REQUIRE(obzcene::overlay::text_width("\n") == 6);
}

TEST_CASE("draw_text paints glyph pixels", "[overlay][text]")
{
  obzcene::FrameBuffer fb{32, 32};
  fb.clear(obzcene::Color{0, 0, 0});
  obzcene::overlay::draw_text(fb, 0, 0, "A", obzcene::Color{255, 255, 255});

  const auto set = count_set(fb, obzcene::Color{0, 0, 0});
  REQUIRE(set > 0);
  REQUIRE(set <= 35);
  for (auto y = 0; y < 32; ++y)
  {
    for (auto x = 0; x < 32; ++x)
    {
      if (fb.at(x, y).r == 255)
      {
        REQUIRE(x < 5);
        REQUIRE(y < 7);
      }
    }
  }
}

TEST_CASE("draw_text handles the full printable range", "[overlay][text]")
{
  auto glyphs = std::string{};
  for (auto c = 0x20; c <= 0x7e; ++c)
  {
    glyphs.push_back(static_cast<char>(c));
  }
  obzcene::FrameBuffer fb{32, 32};
  fb.clear(obzcene::Color{0, 0, 0});
  obzcene::overlay::draw_text(fb, 0, 0, glyphs, obzcene::Color{255, 255, 255});
  REQUIRE(count_set(fb, obzcene::Color{0, 0, 0}) > 0);
}

TEST_CASE("draw_text clips at framebuffer edges", "[overlay][text]")
{
  obzcene::FrameBuffer fb{16, 16};
  fb.clear(obzcene::Color{0, 0, 0});
  obzcene::overlay::draw_text(fb, -8, -8, "HI", obzcene::Color{255, 255, 255});
  REQUIRE(count_set(fb, obzcene::Color{0, 0, 0}) == 0);

  fb.clear(obzcene::Color{0, 0, 0});
  obzcene::overlay::draw_text(fb, 12, 12, "HI", obzcene::Color{255, 255, 255});
  REQUIRE(count_set(fb, obzcene::Color{0, 0, 0}) > 0);
}

TEST_CASE("draw_hud renders a multi-line block", "[overlay][stats]")
{
  obzcene::CameraF cam;
  cam.set_position(Vec3f{1.25f, 2.5f, 3.75f});
  obzcene::FrameBuffer fb{480, 360};
  fb.clear(obzcene::Color{22, 22, 28});

  auto scene   = obzcene::Scene{};
  auto model   = std::make_shared<obzcene::Model>();
  model->mesh  = sample_mesh();
  auto object  = obzcene::SceneObject{};
  object.model = model;
  object.name  = "sample";
  scene.objects.push_back(object);
  auto       stats  = sample_stats();
  const auto lights = sample_lights();

  const auto hud = obzcene::overlay::HudInput{&scene, &stats, &lights, &cam, 60.25f, 16.6f,
                                              false,  1,      true,    0,    "fly",  {}};
  obzcene::overlay::draw_hud(fb, hud);

  const auto set = count_set(fb, obzcene::Color{22, 22, 28});
  REQUIRE(set > 400);
}

TEST_CASE("draw_hud handles an empty scene", "[overlay][stats]")
{
  obzcene::CameraF     cam;
  obzcene::FrameBuffer fb{480, 360};
  fb.clear(obzcene::Color{0, 0, 0});

  auto       scene  = obzcene::Scene{};
  auto       stats  = obzcene::RenderStats{};
  const auto lights = std::vector<obzcene::LightF>{};
  const auto hud    = obzcene::overlay::HudInput{&scene, &stats, &lights, &cam, 0.0f,   0.0f,
                                              true,   0,      false,   -1,   "grab", "hello"};
  obzcene::overlay::draw_hud(fb, hud);

  REQUIRE(count_set(fb, obzcene::Color{0, 0, 0}) > 200);
}