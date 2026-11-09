#include <catch2/catch_all.hpp>

#include <obzcene/Core/Logger.hpp>
#include <obzcene/File/Obj/Parser/ObjParser.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>

static auto assets_dir() -> std::string
{
  const char* env = std::getenv("ASSETS_DIR");
  if (env)
  {
    return std::string(env);
  }
  return "assets";
}

TEST_CASE("Parse cube.obj — basic attributes", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Cube/cube.obj";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.positions.size() == 8);
  REQUIRE(data.texcoords.size() == 14);
  REQUIRE(data.normals.size() == 6);
  REQUIRE(data.objects.size() >= 1);
  REQUIRE_FALSE(data.material_lib.empty());
}

TEST_CASE("Parse cube.obj — faces", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Cube/cube.obj";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.objects.size() == 1);
  auto& obj = data.objects[0];
  REQUIRE(obj.name == "Cube");
  REQUIRE(obj.faces.size() == 12);

  for (const auto& face : obj.faces)
  {
    REQUIRE(face.vertices.size() == 3);
    for (const auto& v : face.vertices)
    {
      REQUIRE(v.position >= 0);
      REQUIRE(std::cmp_less(v.position, data.positions.size()));
      REQUIRE(v.texcoord >= 0);
      REQUIRE(std::cmp_less(v.texcoord, data.texcoords.size()));
      REQUIRE(v.normal >= 0);
      REQUIRE(std::cmp_less(v.normal, data.normals.size()));
    }
  }
}

TEST_CASE("Parse cube.obj — materials loaded from mtllib", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Cube/cube.obj";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.material_lib == "cube.mtl");
}

TEST_CASE("Parse obj with existing mtllib loads materials", "[obj]")
{
  obzcene::Logger::init();

  const std::string obj_content = R"(mtllib test_mtl.mtl
v 0 0 0
v 1 0 0
v 0 1 0
usemtl red_material
f 1 2 3
)";

  const std::string mtl_content = R"(newmtl red_material
Ka 0.1 0.1 0.1
Kd 0.9 0.1 0.1
Ks 0.5 0.5 0.5
Ns 64.0
)";

  std::string base = assets_dir() + "/test_lib_obj";
  {
    std::ofstream out(base + ".obj");
    out << obj_content;
    std::ofstream mtl_out(assets_dir() + "/test_mtl.mtl");
    mtl_out << mtl_content;
  }

  auto data = obzcene::parse_obj(base + ".obj");
  REQUIRE(data.material_lib == "test_mtl.mtl");
  REQUIRE(data.materials.size() == 1);
  REQUIRE(data.materials[0].name == "red_material");

  auto& face = data.objects[0].faces[0];
  REQUIRE(face.material_index == 0);

  std::filesystem::remove(base + ".obj");
  std::filesystem::remove(assets_dir() + "/test_mtl.mtl");
}

TEST_CASE("Parse cube.obj — first vertex position", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Cube/cube.obj";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.positions[0].x == Catch::Approx(1.0F));
  REQUIRE(data.positions[0].y == Catch::Approx(-1.0F));
  REQUIRE(data.positions[0].z == Catch::Approx(-1.0F));
}

TEST_CASE("Parse cube.obj — bounds", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Cube/cube.obj";
  auto data = obzcene::parse_obj(path);

  auto bounds = data.bounds();
  REQUIRE(bounds.min.x == Catch::Approx(-1.0F));
  REQUIRE(bounds.max.x == Catch::Approx(1.0F));
}

TEST_CASE("Parse Skull.obj — multi-object", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Skull/Skull.obj";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.positions.size() > 100);
  REQUIRE(data.normals.size() > 0);
  REQUIRE(data.objects.size() >= 1);

  bool has_faces = false;
  for (const auto& obj : data.objects)
  {
    if (!obj.faces.empty())
    {
      has_faces = true;
      break;
    }
  }
  REQUIRE(has_faces);
}

TEST_CASE("Parse bugatti.obj — large model with usemtl", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/bugatti/bugatti.obj";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.positions.size() > 1000);
  REQUIRE(data.objects.size() > 1);
  REQUIRE(data.materials.size() == 40);
}

TEST_CASE("Parse nonexistent OBJ file returns empty", "[obj]")
{
  obzcene::Logger::init();
  auto data = obzcene::parse_obj("nonexistent.obj");

  REQUIRE(data.positions.empty());
  REQUIRE(data.objects.empty());
}

TEST_CASE("Parse cube.obj — vertex indices are 0-based after resolution", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/Cube/cube.obj";
  auto data = obzcene::parse_obj(path);

  for (const auto& obj : data.objects)
  {
    for (const auto& face : obj.faces)
    {
      for (const auto& v : face.vertices)
      {
        REQUIRE(v.position >= 0);
        REQUIRE(std::cmp_less(v.position, data.positions.size()));
      }
    }
  }
}

TEST_CASE("Parse hand.OBJ — uppercase extension", "[obj]")
{
  obzcene::Logger::init();
  auto path = assets_dir() + "/models/hand/hand.OBJ";
  auto data = obzcene::parse_obj(path);

  REQUIRE(data.positions.size() > 0);
  REQUIRE(data.objects.size() >= 1);
  REQUIRE(data.materials.size() >= 1);
  REQUIRE(data.materials[0].diffuse_texture == "hand_mapNew.jpg");
}

TEST_CASE("Parse face with only positions — no texcoord/normal", "[obj]")
{
  obzcene::Logger::init();

  const std::string obj_content = R"(v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
f 1 2 3
)";

  const std::string mtl_content = "";

  std::string base = assets_dir() + "/test_pos_only";
  {
    std::ofstream out(base + ".obj");
    out << obj_content;
    std::ofstream mtl_out(base + ".mtl");
    mtl_out << mtl_content;
  }

  auto data = obzcene::parse_obj(base + ".obj");
  REQUIRE(data.positions.size() == 3);
  REQUIRE(data.objects.size() == 1);
  REQUIRE(data.objects[0].faces.size() == 1);

  auto& face = data.objects[0].faces[0];
  REQUIRE(face.vertices.size() == 3);
  REQUIRE(face.vertices[0].position == 0);
  REQUIRE(face.vertices[1].position == 1);
  REQUIRE(face.vertices[2].position == 2);
  REQUIRE(face.vertices[0].texcoord == -1);
  REQUIRE(face.vertices[0].normal == -1);

  std::filesystem::remove(base + ".obj");
  std::filesystem::remove(base + ".mtl");
}
