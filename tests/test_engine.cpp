#include <obzcene/Core/Logger.hpp>
#include <obzcene/Engine/Assets.hpp>
#include <obzcene/Engine/Engine.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>

namespace
{

[[nodiscard]] auto assets_root() -> std::filesystem::path
{
  const auto* env = std::getenv("ASSETS_DIR");
  return env != nullptr ? std::filesystem::path{env} : std::filesystem::path{"assets"};
}

} // namespace

TEST_CASE("assets: model resolves by bare name and caches", "[assets]")
{
  obzcene::Logger::init();
  auto assets = obzcene::Assets{assets_root() / "models", true};

  const auto first  = assets.model("Cube/cube.obj");
  const auto second = assets.model("Cube/cube.obj");
  REQUIRE(first != nullptr);
  REQUIRE(first == second); // same shared instance
  REQUIRE(first->mesh.triangles.size() == 12);
}

TEST_CASE("assets: missing model yields nullptr, exists() reports it", "[assets]")
{
  obzcene::Logger::init();
  auto assets = obzcene::Assets{assets_root(), true};
  REQUIRE(assets.exists("models/Cube/cube.obj"));
  REQUIRE_FALSE(assets.exists("models/Cube/nope.obj"));
  REQUIRE(assets.model("models/Cube/nope.obj") == nullptr);
}

TEST_CASE("assets: bare-name lookup adds the extension", "[assets]")
{
  obzcene::Logger::init();
  auto assets = obzcene::Assets{assets_root() / "models", true};
  // "Cube/cube" (no extension) resolves to Cube/cube.obj.
  REQUIRE(assets.model("Cube/cube") != nullptr);
}

TEST_CASE("assets: roots() exposes the search path in priority order", "[assets][debug]")
{
  obzcene::Logger::init();
  auto assets = obzcene::Assets{assets_root() / "models", true};
  REQUIRE(assets.roots().front() == assets_root() / "models");
  REQUIRE(assets.roots().back() == "."); // cwd fallback is always last
  REQUIRE(assets.root() == assets_root() / "models");

  assets.add_root(assets_root() / "audio");
  REQUIRE(assets.roots().back() == assets_root() / "audio");
}

TEST_CASE("assets: OBZCENE_ASSETS_DIR prepends a search root", "[assets][debug]")
{
  obzcene::Logger::init();
  setenv("OBZCENE_ASSETS_DIR", (assets_root() / "models").string().c_str(), 1);
  {
    auto assets = obzcene::Assets{"definitely/wrong", true};
    REQUIRE(assets.roots().front() == assets_root() / "models");
    // The env root resolves what the configured root cannot.
    REQUIRE(assets.model("Cube/cube.obj") != nullptr);
  }
  unsetenv("OBZCENE_ASSETS_DIR");
}

TEST_CASE("engine: config defaults and accessors are wired", "[engine]")
{
  auto engine = obzcene::Engine{{}};
  REQUIRE(engine.scene().objects.empty());
  REQUIRE(engine.dt() == 0.0F);
  REQUIRE(engine.time() == 0.0F);
  REQUIRE(engine.fps() == 0.0F);
  engine.quit();
  REQUIRE_FALSE(engine.is_running());
}

TEST_CASE("engine: game with an asset-loaded object can be constructed", "[engine][assets]")
{
  obzcene::Logger::init();
  auto engine = obzcene::Engine{{}};
  engine.assets().add_root(assets_root() / "models");

  const auto model = engine.assets().model("Cube/cube.obj");
  REQUIRE(model != nullptr);
  const auto object = engine.scene().add_object(model, obzcene::math::Vec3f{0, 0, 0});
  REQUIRE(object != nullptr);
  REQUIRE(engine.scene().objects.size() == 1);
  object->position = obzcene::math::Vec3f{1, 2, 3};
  REQUIRE(engine.scene().objects[0]->position.y == 2.0F);
}
