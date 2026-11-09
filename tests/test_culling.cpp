#include <obzcene/Core/Casts.hpp>
#include <obzcene/File/Obj/Parser/ObjParser.hpp>
#include <obzcene/Math/Vec2.hpp>
#include <obzcene/Math/Vec3.hpp>
#include <obzcene/Renderer/Camera.hpp>
#include <obzcene/Renderer/Color.hpp>
#include <obzcene/Renderer/FrameBuffer.hpp>
#include <obzcene/Renderer/Light.hpp>
#include <obzcene/Renderer/Mesh.hpp>
#include <obzcene/Renderer/Rasterizer.hpp>
#include <obzcene/Renderer/RenderStats.hpp>
#include <obzcene/Renderer/Shader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using obzcene::math::Vec2f;
using obzcene::math::Vec3f;

namespace
{

using obzcene::Mesh;
using obzcene::MeshTriangle;
using obzcene::MeshVertex;

auto make_vertex(const Vec3f& position, const Vec3f& normal, const Vec2f& uv = Vec2f{0, 0})
  -> MeshVertex
{
  return MeshVertex{position, normal, uv};
}

auto make_quad_mesh(bool front_facing = true) -> Mesh
{
  obzcene::Material material;
  material.diffuse  = Vec3f{0.5f, 0.5f, 0.5f};
  material.specular = Vec3f{0, 0, 0};
  material.ambient  = Vec3f{0, 0, 0};

  Mesh mesh;
  mesh.materials.push_back(material);
  const auto vertices = std::vector<MeshVertex>{
    make_vertex(Vec3f{-1, -1, -2}, Vec3f{0, 0, 1}),
    make_vertex(Vec3f{1, -1, -2}, Vec3f{0, 0, 1}),
    make_vertex(Vec3f{1, 1, -2}, Vec3f{0, 0, 1}),
    make_vertex(Vec3f{-1, 1, -2}, Vec3f{0, 0, 1}),
  };
  const auto base = obzcene::narrow_cast<obzcene::ui32>(mesh.vertices.size());
  mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());
  if (front_facing)
  {
    mesh.triangles.push_back(MeshTriangle{base, base + 1, base + 2, 0});
    mesh.triangles.push_back(MeshTriangle{base, base + 2, base + 3, 0});
  }
  else
  {
    mesh.triangles.push_back(MeshTriangle{base + 2, base + 1, base, 0});
    mesh.triangles.push_back(MeshTriangle{base + 3, base + 2, base, 0});
  }
  return mesh;
}

void translate_mesh(Mesh& mesh, const Vec3f& offset)
{
  for (auto& vertex : mesh.vertices)
  {
    vertex.position = vertex.position + offset;
  }
}

void set_depths(Mesh& mesh, float z)
{
  for (auto& vertex : mesh.vertices)
  {
    vertex.position.z = z;
  }
}

auto setup_ortho(obzcene::CameraF& cam) -> void
{
  cam.set_orthographic(-2, 2, -2, 2, 0.1f, 10.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});
}

auto setup_lights() -> std::vector<obzcene::LightF>
{
  return std::vector<obzcene::LightF>{obzcene::LightF::directional(Vec3f{0, 0, -1})};
}

auto count_lit(const obzcene::FrameBuffer& fb, const obzcene::Color& background) -> std::size_t
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

auto rasterize_reference(const Mesh& mesh, const obzcene::FragmentUniforms& uniforms,
                         obzcene::FrameBuffer& fb, const obzcene::RasterOptions& options) -> void
{
  using namespace obzcene::raster_detail;

  const auto vp = uniforms.camera.view_projection();
  for (const auto& tri : mesh.triangles)
  {
    RasterVertex v[3];
    auto         valid = true;
    for (auto i = 0; i < 3; ++i)
    {
      const auto& vertex = mesh.vertices[i == 0 ? tri.a : (i == 1 ? tri.b : tri.c)];
      const auto  clip =
        vp * obzcene::math::Vec4f{vertex.position.x, vertex.position.y, vertex.position.z, 1.0F};
      if (clip.w <= uniforms.camera.near_clip() || clip.w >= uniforms.camera.far_clip())
      {
        valid = false;
        break;
      }
      const auto inv_w = 1.0F / clip.w;
      const auto ndc   = obzcene::math::Vec3f{clip.x * inv_w, clip.y * inv_w, clip.z * inv_w};
      v[i].screen =
        obzcene::math::Vec2f{(ndc.x * 0.5F + 0.5F) * fb.width(), (0.5F - ndc.y * 0.5F) * fb.height()};
      v[i].world  = vertex.position;
      v[i].uv     = vertex.texcoord;
      v[i].normal = vertex.normal;
      v[i].ndc_z  = ndc.z;
      v[i].inv_w  = inv_w;
    }
    if (!valid)
    {
      continue;
    }
    const auto denom = obzcene::math::cross(v[1].screen - v[0].screen, v[2].screen - v[0].screen);
    if (std::abs(denom) < 1.0e-6F)
    {
      continue;
    }
    if (options.backface_cull && denom >= 0.0F)
    {
      continue;
    }

    auto x_min = static_cast<int>(std::min({v[0].screen.x, v[1].screen.x, v[2].screen.x}));
    auto x_max = static_cast<int>(std::max({v[0].screen.x, v[1].screen.x, v[2].screen.x}));
    auto y_min = static_cast<int>(std::min({v[0].screen.y, v[1].screen.y, v[2].screen.y}));
    auto y_max = static_cast<int>(std::max({v[0].screen.y, v[1].screen.y, v[2].screen.y}));
    x_min      = std::max(x_min, 0);
    y_min      = std::max(y_min, 0);
    x_max      = std::min(x_max, fb.width() - 1);
    y_max      = std::min(y_max, fb.height() - 1);

    const auto material_index =
      tri.material_index < mesh.materials.size() ? tri.material_index : 0u;
    const auto& material = mesh.materials[material_index];
    for (auto py = y_min; py <= y_max; ++py)
    {
      for (auto px = x_min; px <= x_max; ++px)
      {
        const auto p  = obzcene::math::Vec2f{px + 0.5F, py + 0.5F};
        const auto wa = obzcene::math::cross(v[2].screen - v[1].screen, p - v[1].screen) / denom;
        const auto wb = obzcene::math::cross(v[0].screen - v[2].screen, p - v[2].screen) / denom;
        const auto wc = 1.0F - wa - wb;
        if (wa < -1.0e-4F || wb < -1.0e-4F || wc < -1.0e-4F)
        {
          continue;
        }
        const auto depth = perspective_blend(wa, wb, wc, v[0].inv_w, v[1].inv_w, v[2].inv_w,
                                             v[0].ndc_z, v[1].ndc_z, v[2].ndc_z);
        if (depth >= fb.depth_at(px, py))
        {
          continue;
        }
        obzcene::FragmentInput input;
        input.world_position = perspective_blend(wa, wb, wc, v[0].inv_w, v[1].inv_w, v[2].inv_w,
                                                 v[0].world, v[1].world, v[2].world);
        input.normal         = obzcene::math::normalized(perspective_blend(
          wa, wb, wc, v[0].inv_w, v[1].inv_w, v[2].inv_w, v[0].normal, v[1].normal, v[2].normal));
        input.uv = perspective_blend(wa, wb, wc, v[0].inv_w, v[1].inv_w, v[2].inv_w, v[0].uv,
                                     v[1].uv, v[2].uv);
        input.material_index = tri.material_index;
        fb.at(px, py)        = obzcene::shade_fragment(input, material, uniforms);
        fb.depth_at(px, py)  = depth;
      }
    }
  }
}

auto make_scene_mesh() -> Mesh
{
  obzcene::Material material;
  material.diffuse  = Vec3f{0.7f, 0.4f, 0.2f};
  material.specular = Vec3f{0, 0, 0};
  material.ambient  = Vec3f{0, 0, 0};

  Mesh mesh;
  mesh.materials.push_back(material);
  const auto push = [&](const MeshVertex& a, const MeshVertex& b, const MeshVertex& c)
  {
    const auto base = obzcene::narrow_cast<obzcene::ui32>(mesh.vertices.size());
    mesh.vertices.push_back(a);
    mesh.vertices.push_back(b);
    mesh.vertices.push_back(c);
    mesh.triangles.push_back(MeshTriangle{base, base + 1, base + 2, 0});
  };

  push(make_vertex(Vec3f{-1.6f, -1.6f, -3}, Vec3f{0, 0, 1}),
       make_vertex(Vec3f{-0.8f, -1.2f, -3}, Vec3f{0, 0, 1}),
       make_vertex(Vec3f{-1.2f, -0.4f, -3}, Vec3f{0, 0, 1}));
  push(make_vertex(Vec3f{0.5f, -1.5f, -2.5f}, Vec3f{0.3f, 0.8f, 0.5f}.normalized()),
       make_vertex(Vec3f{1.6f, -1.1f, -2.5f}, Vec3f{0.3f, 0.8f, 0.5f}.normalized()),
       make_vertex(Vec3f{0.9f, -0.3f, -2.5f}, Vec3f{0.3f, 0.8f, 0.5f}.normalized()));
  push(make_vertex(Vec3f{-1.9f, 0.2f, -2.2f}, Vec3f{0, 0, 1}),
       make_vertex(Vec3f{-1.0f, 0.0f, -2.2f}, Vec3f{0, 0, 1}),
       make_vertex(Vec3f{-1.1f, 1.1f, -2.2f}, Vec3f{0, 0, 1}));
  push(make_vertex(Vec3f{0.1f, 0.3f, -3.2f}, Vec3f{0, 0, -1}),
       make_vertex(Vec3f{1.2f, 0.7f, -3.2f}, Vec3f{0, 0, -1}),
       make_vertex(Vec3f{0.6f, 1.5f, -3.2f}, Vec3f{0, 0, -1}));
  push(make_vertex(Vec3f{-0.3f, -0.4f, -1.6f}, Vec3f{0.2f, 0.5f, 0.8f}.normalized()),
       make_vertex(Vec3f{0.7f, -1.3f, -1.6f}, Vec3f{0.2f, 0.5f, 0.8f}.normalized()),
       make_vertex(Vec3f{1.1f, 0.4f, -1.6f}, Vec3f{0.2f, 0.5f, 0.8f}.normalized()));
  push(make_vertex(Vec3f{-1.4f, -1.8f, -2.8f}, Vec3f{0, 1, 0}),
       make_vertex(Vec3f{-1.9f, 0.9f, -2.8f}, Vec3f{0, 1, 0}),
       make_vertex(Vec3f{0.8f, -1.2f, -2.8f}, Vec3f{0, 1, 0}));
  push(make_vertex(Vec3f{-0.7f, 1.2f, -3.4f}, Vec3f{1, 0, 0}),
       make_vertex(Vec3f{0.9f, 1.4f, -3.4f}, Vec3f{1, 0, 0}),
       make_vertex(Vec3f{0.2f, -0.6f, -3.4f}, Vec3f{1, 0, 0}));
  push(make_vertex(Vec3f{-1.1f, -0.6f, -1.9f}, Vec3f{0, 0, 1}),
       make_vertex(Vec3f{1.8f, -1.4f, -1.9f}, Vec3f{0, 0, 1}),
       make_vertex(Vec3f{1.3f, 1.7f, -1.9f}, Vec3f{0, 0, 1}));
  return mesh;
}

} // namespace

TEST_CASE("backface culling keeps camera-facing triangles", "[raster][cull]")
{
  obzcene::CameraF cam;
  setup_ortho(cam);
  const auto                lights   = setup_lights();
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  const auto stats =
    obzcene::rasterize(make_quad_mesh(true), uniforms, fb, obzcene::RasterOptions{.backface_cull = true});

  REQUIRE(stats.triangles_rendered == 2);
  REQUIRE(stats.triangles_culled_backface == 0);
  REQUIRE(stats.pixels_shaded > 400);
  REQUIRE(stats.pixels_shaded >= stats.pixels_written);
  REQUIRE(stats.pixels_written == count_lit(fb, obzcene::Color{0, 0, 0}));
}

TEST_CASE("backface culling removes rear-facing triangles", "[raster][cull]")
{
  obzcene::CameraF cam;
  setup_ortho(cam);
  const auto                lights   = setup_lights();
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  auto back_facing = make_quad_mesh(false);

  obzcene::FrameBuffer culled_fb{64, 64};
  culled_fb.clear(obzcene::Color{0, 0, 0});
  culled_fb.clear_depth();
  const auto culled =
    obzcene::rasterize(back_facing, uniforms, culled_fb, obzcene::RasterOptions{.backface_cull = true});
  REQUIRE(culled.triangles_culled_backface == 2);
  REQUIRE(culled.triangles_rendered == 0);
  REQUIRE(culled.pixels_shaded == 0);
  REQUIRE(count_lit(culled_fb, obzcene::Color{0, 0, 0}) == 0);

  obzcene::FrameBuffer unculled_fb{64, 64};
  unculled_fb.clear(obzcene::Color{0, 0, 0});
  unculled_fb.clear_depth();
  const auto unculled =
    obzcene::rasterize(back_facing, uniforms, unculled_fb, obzcene::RasterOptions{});
  REQUIRE(unculled.pixels_shaded > 400);
}

TEST_CASE("culling and depth counters stay consistent on a closed shape", "[raster][cull]")
{
  auto cube_data      = obzcene::ObjData{};
  cube_data.positions = {
    Vec3f{-1, -1, -1}, Vec3f{1, -1, -1}, Vec3f{1, -1, 1}, Vec3f{-1, -1, 1},
    Vec3f{-1, 1, -1},  Vec3f{1, 1, -1},  Vec3f{1, 1, 1},  Vec3f{-1, 1, 1},
  };
  obzcene::ObjObject object;
  auto               quad = [&](int a, int b, int c, int d)
  {
    obzcene::ObjFace face;
    face.vertices = {{obzcene::i32{a}, -1, -1},
                     {obzcene::i32{b}, -1, -1},
                     {obzcene::i32{c}, -1, -1},
                     {obzcene::i32{d}, -1, -1}};
    object.faces.push_back(face);
  };
  quad(3, 2, 1, 0);
  quad(4, 5, 6, 7);
  quad(1, 2, 6, 5);
  quad(2, 3, 7, 6);
  quad(0, 1, 5, 4);
  quad(3, 0, 4, 7);
  cube_data.objects.push_back(object);

  const auto mesh = obzcene::build_mesh(cube_data);

  obzcene::CameraF cam;
  cam.set_perspective(obzcene::math::deg_to_rad(50.0f), 1.0f, 0.1f, 20.0f);
  cam.look_at(Vec3f{2, 1.5f, 3}, Vec3f{0, 0, 0});
  const auto                lights   = setup_lights();
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  const auto stats = obzcene::rasterize(mesh, uniforms, fb, obzcene::RasterOptions{.backface_cull = true});

  REQUIRE(stats.triangles_processed == 12);
  REQUIRE(stats.triangles_culled_backface + stats.triangles_rendered == 12);
  REQUIRE(stats.triangles_rendered >= 1);
  REQUIRE(stats.triangles_rendered <= 12);
  REQUIRE(stats.visible_triangles() == stats.triangles_rendered);
  REQUIRE(stats.pixels_written > 0);
}

TEST_CASE("viewport culling rejects offscreen triangles", "[raster][cull]")
{
  auto mesh = make_quad_mesh(true);
  translate_mesh(mesh, Vec3f{50, 0, 0});

  obzcene::CameraF cam;
  setup_ortho(cam);
  const auto                lights   = setup_lights();
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  const auto stats = obzcene::rasterize(mesh, uniforms, fb);

  REQUIRE(stats.triangles_culled_viewport == 2);
  REQUIRE(stats.triangles_rendered == 0);
  REQUIRE(stats.pixels_shaded == 0);
}

TEST_CASE("near and far clipped triangles are counted", "[raster][cull]")
{
  obzcene::CameraF cam;
  cam.set_perspective(obzcene::math::deg_to_rad(60.0f), 1.0f, 0.1f, 10.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});

  auto                            mesh     = make_quad_mesh(true);
  const auto                      lights   = setup_lights();
  const auto                      textures = std::vector<obzcene::Texture>{};
  const obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();

  auto far_mesh = mesh;
  set_depths(far_mesh, -40.0f);
  const auto far_stats = obzcene::rasterize(far_mesh, uniforms, fb);
  REQUIRE(far_stats.triangles_culled_nearfar == 2);
  REQUIRE(far_stats.triangles_rendered == 0);
}

TEST_CASE("degenerate triangles are counted, not rendered", "[raster][cull]")
{
  Mesh              mesh;
  obzcene::Material material;
  material.diffuse  = Vec3f{0.5f, 0.5f, 0.5f};
  material.specular = Vec3f{0, 0, 0};
  material.ambient  = Vec3f{0, 0, 0};
  mesh.materials.push_back(material);
  mesh.vertices.push_back(make_vertex(Vec3f{0, 0, -2}, Vec3f{0, 0, 1}));
  mesh.vertices.push_back(make_vertex(Vec3f{0, 0, -2}, Vec3f{0, 0, 1}));
  mesh.vertices.push_back(make_vertex(Vec3f{1, 0, -2}, Vec3f{0, 0, 1}));
  mesh.triangles.push_back(MeshTriangle{0, 1, 2, 0});

  obzcene::CameraF cam;
  setup_ortho(cam);
  const auto                lights   = setup_lights();
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  const auto stats = obzcene::rasterize(mesh, uniforms, fb);

  REQUIRE(stats.triangles_processed == 1);
  REQUIRE(stats.triangles_degenerate == 1);
  REQUIRE(stats.triangles_rendered == 0);
  REQUIRE(stats.visible_triangles() == 0);
}

TEST_CASE("stats count shaded and written pixels", "[raster][stats]")
{
  obzcene::CameraF cam;
  setup_ortho(cam);
  const auto                lights   = setup_lights();
  const auto                textures = std::vector<obzcene::Texture>{};
  obzcene::FragmentUniforms uniforms{cam, lights, textures};

  obzcene::FrameBuffer fb{64, 64};
  fb.clear(obzcene::Color{0, 0, 0});
  fb.clear_depth();
  const auto stats = obzcene::rasterize(make_quad_mesh(true), uniforms, fb);

  const auto lit = count_lit(fb, obzcene::Color{0, 0, 0});
  REQUIRE(stats.triangles_rendered == 2);
  REQUIRE(stats.pixels_shaded >= lit);
  REQUIRE(stats.pixels_written == lit);
  REQUIRE(lit >= 64 * 64 / 4);
}

TEST_CASE("incremental rasterizer matches per-pixel reference", "[raster][opt]")
{
  const auto mesh     = make_scene_mesh();
  const auto lights   = setup_lights();
  const auto textures = std::vector<obzcene::Texture>{};

  obzcene::CameraF cam;
  cam.set_perspective(obzcene::math::deg_to_rad(60.0f), 1.0f, 0.1f, 30.0f);
  cam.look_at(Vec3f{0, 0, 5}, Vec3f{0, 0, 0});
  const obzcene::FragmentUniforms uniforms{cam, lights, textures};
  const obzcene::RasterOptions    options{};

  obzcene::FrameBuffer optimized{128, 128};
  optimized.clear(obzcene::Color{0, 0, 0});
  optimized.clear_depth();
  const auto stats = obzcene::rasterize(mesh, uniforms, optimized, options);
  REQUIRE(stats.triangles_rendered == mesh.triangles.size());

  obzcene::FrameBuffer reference{128, 128};
  reference.clear(obzcene::Color{0, 0, 0});
  reference.clear_depth();
  rasterize_reference(mesh, uniforms, reference, options);

  const obzcene::Color background{0, 0, 0};
  auto                 mismatches = std::size_t{0};
  for (auto y = 0; y < 128; ++y)
  {
    for (auto x = 0; x < 128; ++x)
    {
      const auto a         = optimized.at(x, y);
      const auto b         = reference.at(x, y);
      const auto covered_a = a.r != background.r || a.g != background.g || a.b != background.b;
      const auto covered_b = b.r != background.r || b.g != background.g || b.b != background.b;
      if (covered_a != covered_b)
      {
        ++mismatches;
        continue;
      }
      if (covered_a)
      {
        REQUIRE(std::abs(static_cast<int>(a.r) - static_cast<int>(b.r)) <= 1);
        REQUIRE(std::abs(static_cast<int>(a.g) - static_cast<int>(b.g)) <= 1);
        REQUIRE(std::abs(static_cast<int>(a.b) - static_cast<int>(b.b)) <= 1);
        REQUIRE(std::abs(optimized.depth_at(x, y) - reference.depth_at(x, y)) <= 1.0e-4f);
      }
    }
  }
  REQUIRE(mismatches <= 16);
}