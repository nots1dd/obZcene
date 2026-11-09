#include <catch2/catch_all.hpp>

#include <obzcene/Core/Logger.hpp>
#include <obzcene/File/Mtl/Parser/MtlParser.hpp>

#include <cstdlib>
#include <filesystem>

static auto assets_dir() -> std::string
{
  const char* env = std::getenv("ASSETS_DIR");
  if (env)
  {
    return std::string(env);
  }
  return "assets";
}

TEST_CASE("Parse legoBrick.mtl — multi-material", "[mtl]")
{
  obzcene::Logger::init();
  auto path      = assets_dir() + "/models/Lego/legoBrick.mtl";
  auto materials = obzcene::parse_mtl(path);

  REQUIRE(materials.size() == 2);

  REQUIRE(materials[0].name == "Material.001");
  REQUIRE(materials[0].shininess == Catch::Approx(96.078431F));
  REQUIRE(materials[0].diffuse.x == Catch::Approx(0.0F));
  REQUIRE(materials[0].diffuse.y == Catch::Approx(0.153556F));
  REQUIRE(materials[0].diffuse.z == Catch::Approx(0.8F));
  REQUIRE(materials[0].specular.x == Catch::Approx(0.421875F));
  REQUIRE(materials[0].opacity == Catch::Approx(1.0F));

  REQUIRE(materials[1].name == "Material.002");
  REQUIRE(materials[1].diffuse.x == Catch::Approx(1.0F));
  REQUIRE(materials[1].diffuse.y == Catch::Approx(1.0F));
  REQUIRE(materials[1].diffuse.z == Catch::Approx(1.0F));
}

TEST_CASE("Parse Skull.mtl — has map_Kd and map_Ka", "[mtl]")
{
  obzcene::Logger::init();
  auto path      = assets_dir() + "/models/Skull/12140_Skull_v3_L2.mtl";
  auto materials = obzcene::parse_mtl(path);

  REQUIRE(materials.size() == 1);

  auto& mat = materials[0];
  REQUIRE(mat.name == "12140_Skull_v3");
  REQUIRE(mat.shininess == Catch::Approx(24.0F));
  REQUIRE(mat.ior == Catch::Approx(1.5F));
  REQUIRE(mat.opacity == Catch::Approx(1.0F));
  REQUIRE(mat.ambient.x == Catch::Approx(1.0F));
  REQUIRE(mat.diffuse.x == Catch::Approx(1.0F));
  REQUIRE(mat.specular.x == Catch::Approx(0.117F));
  REQUIRE(mat.diffuse_texture == "Skull.png");
  REQUIRE(mat.ambient_texture == "Skull.png");
}

TEST_CASE("Parse Hand.mtl — has map_Kd", "[mtl]")
{
  obzcene::Logger::init();
  auto path      = assets_dir() + "/models/hand/Hand.mtl";
  auto materials = obzcene::parse_mtl(path);

  REQUIRE(materials.size() == 1);

  auto& mat = materials[0];
  REQUIRE(mat.name == "defaultMat");
  REQUIRE(mat.diffuse.x == Catch::Approx(0.64F));
  REQUIRE(mat.diffuse_texture == "hand_mapNew.jpg");
}

TEST_CASE("Parse nonexistent MTL file returns empty", "[mtl]")
{
  obzcene::Logger::init();
  auto materials = obzcene::parse_mtl("nonexistent.mtl");
  REQUIRE(materials.empty());
}

TEST_CASE("Parse MTL with Tr (transparency) field", "[mtl]")
{
  obzcene::Logger::init();

  const std::string mtl_content = R"(newmtl glass
  Ka 0.1 0.1 0.1
  Kd 0.5 0.5 0.5
  Ks 1.0 1.0 1.0
  Ns 200.0
  Tr 0.5
  illum 2
)";

  auto path = assets_dir() + "/test_tr.mtl";
  {
    std::ofstream out(path);
    out << mtl_content;
  }

  auto materials = obzcene::parse_mtl(path);
  REQUIRE(materials.size() == 1);
  REQUIRE(materials[0].name == "glass");
  REQUIRE(materials[0].opacity == Catch::Approx(0.5F));

  std::filesystem::remove(path);
}

TEST_CASE("Parse bugatti.mtl — many materials", "[mtl]")
{
  obzcene::Logger::init();
  auto path      = assets_dir() + "/models/bugatti/bugatti.mtl";
  auto materials = obzcene::parse_mtl(path);

  REQUIRE(materials.size() == 40);

  REQUIRE(materials[0].name == "BLUE.002");
  REQUIRE(materials[0].diffuse.z == Catch::Approx(0.8F));

  REQUIRE(materials[1].name == "ENGINE");
  REQUIRE(materials[1].diffuse.x == Catch::Approx(0.64F));
}
