#include <obzcene/Core/Logger.hpp>
#include <obzcene/Viewer/FileBrowser.hpp>
#include <obzcene/Viewer/Scene.hpp>
#include <obzcene/Viewer/SceneFile.hpp>
#include <obzcene/Viewer/SceneObject.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using obzcene::math::Vec3f;

namespace
{

auto temp_dir(const std::string& name) -> std::filesystem::path
{
  const auto dir = std::filesystem::temp_directory_path() / name;
  std::filesystem::create_directories(dir);
  return dir;
}

} // namespace

TEST_CASE("SceneObject model matrix applies T*R*S about the origin", "[scene][transform]")
{
  auto object      = obzcene::SceneObject{};
  object.position  = Vec3f{10, 20, 30};
  object.yaw       = 0.0F;
  object.scale     = 2.0F;
  const auto m     = object.model_matrix();
  const auto point = m * obzcene::math::Vec4f{1, 0, 0, 1};
  REQUIRE(point.x == Catch::Approx(12.0F));
  REQUIRE(point.y == Catch::Approx(20.0F));
  REQUIRE(point.z == Catch::Approx(30.0F));
}

TEST_CASE("SceneObject yaw rotates about the position", "[scene][transform]")
{
  auto object     = obzcene::SceneObject{};
  object.position = Vec3f{5, 0, 0};
  object.yaw      = obzcene::math::deg_to_rad(90.0F);
  const auto m    = object.model_matrix();
  const auto p    = m * obzcene::math::Vec4f{1, 0, 0, 1};
  REQUIRE(p.x == Catch::Approx(5.0F).margin(1.0e-4F));
  REQUIRE(p.z == Catch::Approx(-1.0F).margin(1.0e-4F));
}

TEST_CASE("SceneObject transformed bounds enclose the transformed corners", "[scene][transform]")
{
  auto object      = obzcene::SceneObject{};
  object.position  = Vec3f{100, 0, 0};
  object.scale     = 2.0F;
  const auto local = obzcene::math::AABBf{Vec3f{-1, -1, -1}, Vec3f{1, 1, 1}};
  const auto world = object.transformed_bounds(local);
  REQUIRE(world.min.x == Catch::Approx(98.0F));
  REQUIRE(world.max.x == Catch::Approx(102.0F));
  REQUIRE(world.max.y == Catch::Approx(2.0F));
}

TEST_CASE("scene file round-trips objects, lights and camera", "[scene][io]")
{
  const auto dir  = temp_dir("obz_scene_io");
  const auto path = dir / "test.obzscene";

  auto scene = obzcene::Scene{};
  auto model = std::make_shared<obzcene::Model>();
  model->mesh.vertices.resize(3);
  const auto index   = scene.add_object(model, Vec3f{1, 2, 3});
  auto&      object  = scene.objects[index];
  object.yaw         = 0.5F;
  object.scale       = 2.5F;
  object.name        = "cube.obj";
  object.source_path = "models/cube.obj";
  scene.lights.push_back(obzcene::LightF::point(Vec3f{1, 5, 2}, Vec3f{1, 0.5F, 0.25F}, 2.0F));
  scene.lights.push_back(obzcene::LightF::directional(Vec3f{0, -1, 0}, Vec3f{1, 1, 1}, 0.8F));
  scene.lights.push_back(
    obzcene::LightF::spot(Vec3f{0, 3, 0}, Vec3f{0, -1, 0}, Vec3f{0.9F, 0.9F, 1.0F}, 1.2F));

  obzcene::CameraF cam;
  cam.set_position(Vec3f{7, 8, 9});
  cam.set_orientation(0.3F, -0.2F);

  REQUIRE(obzcene::save_scene(scene, path, cam));

  const auto loaded = obzcene::load_scene_file(path);
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->objects.size() == 1);
  REQUIRE(loaded->objects[0].model_path == "models/cube.obj");
  REQUIRE(loaded->objects[0].position.x == Catch::Approx(1.0F));
  REQUIRE(loaded->objects[0].position.y == Catch::Approx(2.0F));
  REQUIRE(loaded->objects[0].position.z == Catch::Approx(3.0F));
  REQUIRE(loaded->objects[0].yaw == Catch::Approx(0.5F));
  REQUIRE(loaded->objects[0].scale == Catch::Approx(2.5F));

  REQUIRE(loaded->lights.size() == 3);
  REQUIRE(loaded->lights[0].type == obzcene::LightType::Point);
  REQUIRE(loaded->lights[0].position.y == Catch::Approx(5.0F));
  REQUIRE(loaded->lights[0].intensity == Catch::Approx(2.0F));
  REQUIRE(loaded->lights[1].type == obzcene::LightType::Directional);
  REQUIRE(loaded->lights[2].type == obzcene::LightType::Spot);
  REQUIRE(loaded->lights[2].direction.y == Catch::Approx(-1.0F));

  REQUIRE(loaded->camera_position.has_value());
  REQUIRE(loaded->camera_position->x == Catch::Approx(7.0F));
  REQUIRE(loaded->camera_yaw.value() == Catch::Approx(0.3F));
  REQUIRE(loaded->camera_pitch.value() == Catch::Approx(-0.2F));

  std::filesystem::remove(path);
}

TEST_CASE("scene file parser skips malformed lines", "[scene][io]")
{
  const auto dir  = temp_dir("obz_scene_bad");
  const auto path = dir / "bad.obzscene";
  {
    std::ofstream out(path);
    out << "# comment\n"
           "\n"
           "version 1\n"
           "object\n"
           "object model.obj 1 2 3 0 1\n"
           "light bogus 0 0 0 1 1 1 1\n"
           "light point 0 5 0 1 1 1 1\n"
           "camera 1 2 3 0.1 0.2\n"
           "nonsense line here\n";
  }
  const auto loaded = obzcene::load_scene_file(path);
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->objects.size() == 1);
  REQUIRE(loaded->lights.size() == 1);
  REQUIRE(loaded->camera_position.has_value());
  std::filesystem::remove(path);
}

TEST_CASE("scene file on a missing path yields nullopt", "[scene][io]")
{
  REQUIRE_FALSE(obzcene::load_scene_file("/definitely/not/here.obzscene").has_value());
}

TEST_CASE("file browser lists dirs before files and filters by extension", "[browser]")
{
  const auto dir = temp_dir("obz_browser");
  std::filesystem::create_directories(dir / "subdir");
  {
    std::ofstream out(dir / "a.obj");
  }
  {
    std::ofstream out(dir / "b.obj");
  }
  {
    std::ofstream out(dir / "notes.txt");
  }

  auto browser = obzcene::FileBrowser{};
  browser.open(dir, ".obj");
  REQUIRE(browser.is_open());
  REQUIRE(browser.entries().size() == 3); // subdir + 2 obj files; notes.txt filtered
  REQUIRE(browser.entries()[0].is_dir);
  REQUIRE(browser.entries()[0].name == "subdir");
  REQUIRE(browser.entries()[1].name == "a.obj");
  REQUIRE(browser.entries()[2].name == "b.obj");

  std::filesystem::remove_all(dir);
}

TEST_CASE("file browser navigation: descend, parent, pick", "[browser]")
{
  const auto dir = temp_dir("obz_browser_nav");
  std::filesystem::create_directories(dir / "models");
  {
    std::ofstream out(dir / "models" / "cube.obj");
  }
  {
    std::ofstream out(dir / "root.obj");
  }

  auto browser = obzcene::FileBrowser{};
  browser.open(dir, ".obj");

  // Cursor starts on "models/" (dirs first); enter descends.
  browser.activate();
  REQUIRE(browser.is_open());
  REQUIRE(browser.current_dir().filename() == "models");
  REQUIRE(browser.entries().size() == 1);

  // Pick the file: closes and records the path.
  browser.activate();
  REQUIRE_FALSE(browser.is_open());
  const auto picked = browser.take_selected();
  REQUIRE(picked.has_value());
  REQUIRE(picked->filename() == "cube.obj");
  REQUIRE_FALSE(browser.take_selected().has_value()); // consumed

  // Reopen and go to parent.
  browser.open(dir / "models", ".obj");
  browser.go_parent();
  REQUIRE(browser.current_dir() == dir);

  std::filesystem::remove_all(dir);
}

TEST_CASE("file browser cursor clamps at the ends", "[browser]")
{
  const auto dir = temp_dir("obz_browser_clamp");
  {
    std::ofstream out(dir / "a.obj");
  }
  {
    std::ofstream out(dir / "b.obj");
  }

  auto browser = obzcene::FileBrowser{};
  browser.open(dir, ".obj");
  browser.move_cursor(-10);
  REQUIRE(browser.cursor() == 0);
  browser.move_cursor(10);
  REQUIRE(browser.cursor() == 1);

  std::filesystem::remove_all(dir);
}

TEST_CASE("scene load_model caches models (instancing)", "[scene][model]")
{
  obzcene::Logger::init();
  auto        scene = obzcene::Scene{};
  const auto* env   = std::getenv("ASSETS_DIR");
  const auto  cube =
    (env != nullptr ? std::filesystem::path{env} : std::filesystem::path{"assets"}) / "models" /
    "Cube" / "cube.obj";

  const auto first  = scene.load_model(cube, true);
  const auto second = scene.load_model(cube, true);
  REQUIRE(first != nullptr);
  REQUIRE(first == second); // same shared_ptr: instanced, not reloaded

  const auto a = scene.add_object(first, Vec3f{0, 0, 0});
  const auto b = scene.add_object(second, Vec3f{5, 0, 0});
  REQUIRE(scene.instance_count(first) == 2);
  REQUIRE(scene.objects[a].model == scene.objects[b].model);
}
