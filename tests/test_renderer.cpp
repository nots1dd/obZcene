#include <obzcene/Core/Casts.hpp>
#include <obzcene/Core/Types.hpp>
#include <obzcene/File/Obj/Parser/ObjParser.hpp>
#include <obzcene/Math/Vec2.hpp>
#include <obzcene/Math/Vec3.hpp>
#include <obzcene/Renderer/Camera.hpp>
#include <obzcene/Renderer/Color.hpp>
#include <obzcene/Renderer/FrameBuffer.hpp>
#include <obzcene/Renderer/Light.hpp>
#include <obzcene/Renderer/Mesh.hpp>
#include <obzcene/Renderer/Rasterizer.hpp>
#include <obzcene/Renderer/Shader.hpp>
#include <obzcene/Texture/Texture.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using obzcene::math::Vec2f;
using obzcene::math::Vec2i;
using obzcene::math::Vec3f;

namespace
{

auto asset_path(const std::string& relative) -> std::filesystem::path
{
  const auto* env  = std::getenv("ASSETS_DIR");
  const auto  root = env != nullptr ? std::filesystem::path{env} : std::filesystem::path{"assets"};
  return root / relative;
}

auto make_quad_mesh(obzcene::Material material) -> obzcene::Mesh
{
  obzcene::Mesh mesh;
  mesh.materials.push_back(material);
  const auto make_vertex = [](const Vec3f& position, const Vec3f& normal, const Vec2f& uv)
  { return obzcene::MeshVertex{position, normal, uv}; };
  const auto verts = std::vector<obzcene::MeshVertex>{
    make_vertex(Vec3f{-1, -1, -2}, Vec3f{0, 0, 1}, Vec2f{0, 0}),
    make_vertex(Vec3f{1, -1, -2}, Vec3f{0, 0, 1}, Vec2f{1, 0}),
    make_vertex(Vec3f{1, 1, -2}, Vec3f{0, 0, 1}, Vec2f{1, 1}),
    make_vertex(Vec3f{-1, 1, -2}, Vec3f{0, 0, 1}, Vec2f{0, 1}),
  };
  const auto base = obzcene::narrow_cast<obzcene::ui32>(mesh.vertices.size());
  mesh.vertices.insert(mesh.vertices.end(), verts.begin(), verts.end());
  mesh.triangles.push_back(obzcene::MeshTriangle{base, base + 1, base + 2, 0});
  mesh.triangles.push_back(obzcene::MeshTriangle{base, base + 2, base + 3, 0});
  return mesh;
}

auto make_gradient_texture(int samples) -> obzcene::Texture
{
  obzcene::Texture tex;
  tex.channels = 3;
  tex.width    = samples;
  tex.height   = 1;
  tex.data.resize(static_cast<obzcene::usz>(samples) * 3);
  for (auto i = 0; i < samples; ++i)
  {
    const auto value =
      static_cast<obzcene::ui8>(static_cast<float>(i) / (samples - 1) * 255.0f + 0.5f);
    tex.data[static_cast<obzcene::usz>(i) * 3]     = value;
    tex.data[static_cast<obzcene::usz>(i) * 3 + 1] = value;
    tex.data[static_cast<obzcene::usz>(i) * 3 + 2] = value;
  }
  return tex;
}

auto brightness(const obzcene::Color& c) -> int
{
  return (static_cast<int>(c.r) + static_cast<int>(c.g) + static_cast<int>(c.b)) / 3;
}

auto solid_lit_camera(obzcene::CameraF& cam) -> void
{
  cam.set_perspective(obzcene::math::deg_to_rad(45.0f), 1.0f, 0.1f, 100.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});
}

} // namespace

TEST_CASE("Color converts from normalized components", "[renderer][color]")
{
  const auto white = obzcene::Color::from_normalized(Vec3f{1, 1, 1});
  REQUIRE(white.r == 255);
  REQUIRE(white.g == 255);
  REQUIRE(white.b == 255);

  const auto black = obzcene::Color::from_normalized(Vec3f{0, 0, 0});
  REQUIRE(black.r == 0);
  REQUIRE(black.g == 0);
  REQUIRE(black.b == 0);

  const auto clamped = obzcene::Color::from_normalized(Vec3f{2, -1, 0.5f});
  REQUIRE(clamped.r == 255);
  REQUIRE(clamped.g == 0);
  REQUIRE(clamped.b == static_cast<obzcene::ui8>(0.5f * 255.0f + 0.5f));
}

TEST_CASE("FrameBuffer tracks color and depth", "[renderer][framebuffer]")
{
  obzcene::FrameBuffer fb{4, 3};
  REQUIRE(fb.width() == 4);
  REQUIRE(fb.height() == 3);
  REQUIRE(fb.inside(0, 0));
  REQUIRE(fb.inside(3, 2));
  REQUIRE(!fb.inside(4, 2));
  REQUIRE(!fb.inside(0, -1));

  fb.clear(obzcene::Color{10, 20, 30});
  fb.clear_depth(7.0f);
  REQUIRE(fb.at(2, 1).r == 10);
  REQUIRE(fb.depth_at(2, 1) == Catch::Approx(7.0f));
  fb.at(0, 0)       = obzcene::Color{1, 2, 3};
  fb.depth_at(0, 0) = 0.5f;
  REQUIRE(fb.at(0, 0).r == 1);
  REQUIRE(fb.depth_at(0, 0) == Catch::Approx(0.5f));
  REQUIRE(fb.at(3, 2).r == 10);
}

TEST_CASE("checkerboard texture sampling", "[renderer][texture]")
{
  const auto tex =
    obzcene::make_checkerboard(16, 16, 8, obzcene::Color{255, 0, 0}, obzcene::Color{0, 0, 255});
  REQUIRE(tex.width == 16);
  REQUIRE(tex.height == 16);
  REQUIRE(tex.channels == 3);

  const auto white_cell = obzcene::sample_nearest(tex, 0.1f, 0.1f, obzcene::TextureWrap::Clamp);
  const auto black_cell = obzcene::sample_nearest(tex, 0.9f, 0.1f, obzcene::TextureWrap::Clamp);
  REQUIRE(white_cell.x == Catch::Approx(1.0f));
  REQUIRE(white_cell.z == Catch::Approx(0.0f));
  REQUIRE(black_cell.z == Catch::Approx(1.0f));
  REQUIRE(black_cell.x == Catch::Approx(0.0f));
}

TEST_CASE("texture wrap modes", "[renderer][texture]")
{
  const auto tex = obzcene::make_solid(8, 8, obzcene::Color{200, 100, 50});

  const auto repeat = obzcene::sample_nearest(tex, -0.1f, 0.5f, obzcene::TextureWrap::Repeat);
  REQUIRE(repeat.x == Catch::Approx(200.0f / 255.0f));

  const auto clamped = obzcene::sample_nearest(tex, 1.5f, 0.5f, obzcene::TextureWrap::Clamp);
  REQUIRE(clamped.x == Catch::Approx(200.0f / 255.0f));

  const auto mirrored = obzcene::sample_nearest(tex, 1.1f, 0.5f, obzcene::TextureWrap::Mirror);
  REQUIRE(mirrored.x == Catch::Approx(200.0f / 255.0f));
}

TEST_CASE("linear sampling interpolates between texels", "[renderer][texture]")
{
  const auto tex = [&]()
  {
    obzcene::Texture t;
    t.channels = 3;
    t.width    = 2;
    t.height   = 1;
    t.data     = {0, 0, 0, 255, 255, 255};
    return t;
  }();
  const auto middle = obzcene::sample_linear(tex, 0.5f, 0.5f, obzcene::TextureWrap::Clamp);
  REQUIRE(middle.x == Catch::Approx(0.5f).margin(0.02f));
  REQUIRE(middle.y == Catch::Approx(0.5f).margin(0.02f));
}

TEST_CASE("build_mesh fan-triangulates a quad from ObjData", "[renderer][mesh]")
{
  auto data      = obzcene::ObjData{};
  data.positions = {Vec3f{0, 0, 0}, Vec3f{1, 0, 0}, Vec3f{1, 1, 0}, Vec3f{0, 1, 0}};
  data.texcoords = {Vec2f{0, 0}, Vec2f{1, 0}, Vec2f{1, 1}, Vec2f{0, 1}};
  obzcene::Material mat;
  mat.name = "quad_mat";
  data.materials.push_back(mat);

  obzcene::ObjObject object;
  object.name = "quad";
  obzcene::ObjFace face;
  face.material_index = 0;
  face.vertices       = {{0, 0, -1}, {1, 1, -1}, {2, 2, -1}, {3, 3, -1}};
  object.faces.push_back(face);
  data.objects.push_back(object);

  const auto mesh = obzcene::build_mesh(data);
  REQUIRE(mesh.triangles.size() == 2);
  // Corners are shared through the vertex cache: a quad is 4 unique vertices.
  REQUIRE(mesh.vertices.size() == 4);
  REQUIRE(mesh.materials.size() == 1);

  const auto& v0 = mesh.vertices[0];
  REQUIRE(v0.position == Vec3f{0, 0, 0});
  REQUIRE(v0.texcoord == Vec2f{0, 0});
  REQUIRE(v0.normal == Vec3f{0, 0, 1});

  REQUIRE(mesh.triangles[0].material_index == 0);
  REQUIRE(mesh.triangles[0].a == 0);
  REQUIRE(mesh.triangles[0].b == 1);
  REQUIRE(mesh.triangles[0].c == 2);
  REQUIRE(mesh.triangles[1].a == 0);
  REQUIRE(mesh.triangles[1].b == 2);
  REQUIRE(mesh.triangles[1].c == 3);
}

TEST_CASE("build_mesh falls back to face normal when normals are missing", "[renderer][mesh]")
{
  auto data      = obzcene::ObjData{};
  data.positions = {Vec3f{0, 0, 0}, Vec3f{1, 0, 0}, Vec3f{1, 1, 0}, Vec3f{0, 1, 0}};
  obzcene::ObjObject object;
  obzcene::ObjFace   face;
  face.vertices = {{0, -1, -1}, {1, -1, -1}, {2, -1, -1}, {3, -1, -1}};
  object.faces.push_back(face);
  data.objects.push_back(object);

  const auto mesh = obzcene::build_mesh(data);
  REQUIRE(mesh.triangles.size() == 2);
  for (const auto& vertex : mesh.vertices)
  {
    REQUIRE(vertex.texcoord == Vec2f{0, 0});
    REQUIRE(vertex.normal == Vec3f{0, 0, 1});
  }
}

TEST_CASE("build_mesh triangulates a pentagon", "[renderer][mesh]")
{
  auto data      = obzcene::ObjData{};
  data.positions = {Vec3f{0, 0, 0}, Vec3f{1, 0, 0}, Vec3f{1.5f, 1, 0}, Vec3f{0.5f, 2, 0},
                    Vec3f{-0.5f, 1, 0}};
  obzcene::ObjObject object;
  obzcene::ObjFace   face;
  face.vertices = {{0, -1, -1}, {1, -1, -1}, {2, -1, -1}, {3, -1, -1}, {4, -1, -1}};
  object.faces.push_back(face);
  data.objects.push_back(object);

  const auto mesh = obzcene::build_mesh(data);
  REQUIRE(mesh.triangles.size() == 3);
  // Fan around corner 0 over 5 unique positions: 5 cached vertices.
  REQUIRE(mesh.vertices.size() == 5);
  REQUIRE(mesh.triangles[1].a == mesh.triangles[0].a);
  REQUIRE(mesh.triangles[2].a == mesh.triangles[0].a);
}

TEST_CASE("build_mesh matches face counts of assets", "[renderer][mesh][assets]")
{
  const auto cube      = obzcene::parse_obj(asset_path("models/Cube/cube.obj").string());
  const auto cube_mesh = obzcene::build_mesh(cube);
  REQUIRE(cube_mesh.triangles.size() == 12);
  // 36 face corners collapse to 24 unique (position, texcoord, normal) triples.
  REQUIRE(cube_mesh.vertices.size() == 24);

  const auto hand      = obzcene::parse_obj(asset_path("models/hand/hand.OBJ").string());
  const auto hand_mesh = obzcene::build_mesh(hand);
  REQUIRE(hand_mesh.triangles.size() == 8808);
  // Vertex cache: the pool holds each unique corner once, never 3x the faces.
  REQUIRE(hand_mesh.vertices.size() > 0);
  REQUIRE(hand_mesh.vertices.size() <= 8808 * 3);

  const auto skull      = obzcene::parse_obj(asset_path("models/Skull/Skull.obj").string());
  const auto skull_mesh = obzcene::build_mesh(skull);
  REQUIRE(skull_mesh.triangles.size() >= 10000);
  REQUIRE(skull_mesh.materials.size() == skull.materials.size());
}

TEST_CASE("rasterizer draws a fullscreen quad", "[renderer][raster]")
{
  obzcene::Material mat;
  mat.diffuse     = Vec3f{0.2f, 0.4f, 0.6f};
  mat.specular    = Vec3f{0, 0, 0};
  mat.ambient     = Vec3f{0, 0, 0};
  const auto mesh = make_quad_mesh(mat);

  obzcene::CameraF cam;
  cam.set_orthographic(-1, 1, -1, 1, 0.1f, 10.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});

  const auto                light    = obzcene::LightF::directional(Vec3f{0, 0, -1});
  const auto                lights   = std::vector<obzcene::LightF>{light};
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{128, 128};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  obzcene::rasterize(mesh, uniforms, fb);

  auto lit = 0;
  for (auto y = 0; y < fb.height(); ++y)
  {
    for (auto x = 0; x < fb.width(); ++x)
    {
      if (fb.at(x, y).r > 40)
      {
        ++lit;
      }
    }
  }
  REQUIRE(lit > 128 * 128 * 9 / 10);

  const auto center = fb.at(64, 64);
  REQUIRE(center.r == Catch::Approx(0.2f * 255.0f).margin(12.0f));
  REQUIRE(center.g == Catch::Approx(0.4f * 255.0f).margin(12.0f));
  REQUIRE(center.b == Catch::Approx(0.6f * 255.0f).margin(12.0f));
}

TEST_CASE("rasterizer interpolates uv across a gradient texture", "[renderer][raster]")
{
  obzcene::Material mat;
  mat.diffuse     = Vec3f{1, 1, 1};
  mat.specular    = Vec3f{0, 0, 0};
  mat.ambient     = Vec3f{0, 0, 0};
  const auto mesh = make_quad_mesh(mat);

  obzcene::CameraF cam;
  cam.set_orthographic(-1, 1, -1, 1, 0.1f, 10.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});

  const auto                light    = obzcene::LightF::directional(Vec3f{0, 0, -1});
  const auto                gradient = make_gradient_texture(8);
  const auto                lights   = std::vector<obzcene::LightF>{light};
  const auto                textures = std::vector<obzcene::Texture>{gradient};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{128, 128};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  obzcene::rasterize(mesh, uniforms, fb);

  const auto at = [&](int x) -> obzcene::Color { return fb.at(x, 64); };
  REQUIRE(brightness(at(64)) == Catch::Approx(127.5f).margin(15.0f));
  REQUIRE(brightness(at(32)) == Catch::Approx(64.0f).margin(15.0f));
  REQUIRE(brightness(at(96)) == Catch::Approx(191.0f).margin(15.0f));
}

TEST_CASE("depth buffer occludes far geometry", "[renderer][depth]")
{
  obzcene::Material red;
  red.diffuse  = Vec3f{1, 0, 0};
  red.specular = Vec3f{0, 0, 0};
  red.ambient  = Vec3f{0, 0, 0};
  obzcene::Material blue;
  blue.diffuse  = Vec3f{0, 0, 1};
  blue.specular = Vec3f{0, 0, 0};
  blue.ambient  = Vec3f{0, 0, 0};

  auto near_mesh = make_quad_mesh(red);
  for (auto& v : near_mesh.vertices)
  {
    v.position = Vec3f{v.position.x * 1.5f, v.position.y * 1.5f, -1.0f};
  }
  auto far_mesh = make_quad_mesh(blue);
  for (auto& v : far_mesh.vertices)
  {
    v.position = Vec3f{v.position.x * 8.0f, v.position.y * 8.0f, -8.0f};
  }

  obzcene::CameraF cam;
  solid_lit_camera(cam);
  const auto lights   = std::vector<obzcene::LightF>{obzcene::LightF::directional(Vec3f{0, 0, -1})};
  const auto textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  const auto render_both = [&](bool near_first)
  {
    obzcene::FrameBuffer fb{64, 64};
    fb.clear(obzcene::Color{0, 0, 0});
    fb.clear_depth();
    if (near_first)
    {
      obzcene::rasterize(near_mesh, uniforms, fb);
      obzcene::rasterize(far_mesh, uniforms, fb);
    }
    else
    {
      obzcene::rasterize(far_mesh, uniforms, fb);
      obzcene::rasterize(near_mesh, uniforms, fb);
    }
    return fb;
  };

  for (const auto near_first : {true, false})
  {
    const auto fb     = render_both(near_first);
    const auto center = fb.at(32, 32);
    REQUIRE(center.r >= 200);
    REQUIRE(center.b < 80);
    const auto edge = fb.at(2, 2);
    REQUIRE(edge.b > 180);
  }
}

TEST_CASE("material-less models get a default material and render", "[renderer][mesh]")
{
  auto data      = obzcene::ObjData{};
  data.positions = {Vec3f{-1, -1, 0}, Vec3f{1, -1, 0}, Vec3f{1, 1, 0}, Vec3f{-1, 1, 0}};
  obzcene::ObjObject object;
  obzcene::ObjFace   face;
  face.vertices = {{0, -1, -1}, {1, -1, -1}, {2, -1, -1}, {3, -1, -1}};
  object.faces.push_back(face);
  data.objects.push_back(object);

  const auto mesh = obzcene::build_mesh(data);
  REQUIRE(mesh.materials.size() == 1);
  REQUIRE(mesh.triangles.size() == 2);

  obzcene::CameraF cam;
  cam.set_orthographic(-2, 2, -2, 2, 0.1f, 10.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});

  const auto lights   = std::vector<obzcene::LightF>{obzcene::LightF::directional(Vec3f{0, 0, -1})};
  const auto textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  obzcene::rasterize(mesh, uniforms, fb);

  auto lit = 0;
  for (auto y = 0; y < fb.height(); ++y)
  {
    for (auto x = 0; x < fb.width(); ++x)
    {
      if (fb.at(x, y).r > 40)
      {
        ++lit;
      }
    }
  }
  REQUIRE(lit >= 64 * 64 / 4);
}

TEST_CASE("fragment shading responds to light direction", "[renderer][shade]")
{
  auto mesh = make_quad_mesh(
    []()
    {
      obzcene::Material m;
      m.diffuse  = Vec3f{0.5f, 0.5f, 0.5f};
      m.specular = Vec3f{0, 0, 0};
      m.ambient  = Vec3f{0, 0, 0};
      return m;
    }());

  obzcene::CameraF cam;
  solid_lit_camera(cam);

  const auto right_light = obzcene::LightF::directional(Vec3f{-1, 0, 1}.normalized());
  const auto lights      = std::vector<obzcene::LightF>{right_light};
  const auto textures    = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  obzcene::rasterize(mesh, uniforms, fb);

  const auto center = fb.at(32, 32);
  REQUIRE(brightness(center) < 20);
}