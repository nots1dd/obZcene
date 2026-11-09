#include <catch2/catch_all.hpp>

#include <obzcene/Core/Logger.hpp>

TEST_CASE("Logger initializes without crashing", "[logger]")
{
  REQUIRE_NOTHROW(obzcene::Logger::init());
}

TEST_CASE("Logger core instance is valid after init", "[logger]")
{
  obzcene::Logger::init();
  auto& logger = obzcene::Logger::get();
  REQUIRE(logger != nullptr);
  REQUIRE(logger->name() == "CORE");
}

TEST_CASE("Logger module loggers are created on demand", "[logger]")
{
  obzcene::Logger::init();

  auto obj_logger = obzcene::Logger::get("OBJ");
  REQUIRE(obj_logger != nullptr);
  REQUIRE(obj_logger->name() == "OBJ");

  auto mtl_logger = obzcene::Logger::get("MTL");
  REQUIRE(mtl_logger != nullptr);
  REQUIRE(mtl_logger->name() == "MTL");

  auto same_obj = obzcene::Logger::get("OBJ");
  REQUIRE(obj_logger == same_obj);
}

TEST_CASE("Logger macros compile and execute", "[logger]")
{
  obzcene::Logger::init();

  REQUIRE_NOTHROW(LOG_TRACE("trace message {}", 1));
  REQUIRE_NOTHROW(LOG_INFO("info message"));
  REQUIRE_NOTHROW(LOG_WARN("warn message"));
  REQUIRE_NOTHROW(LOG_ERROR("error message"));
  REQUIRE_NOTHROW(LOG_CRITICAL("critical message"));

  REQUIRE_NOTHROW(LOG_MODULE_INFO("OBJ", "module info {}", "test"));
  REQUIRE_NOTHROW(LOG_MODULE_WARN("MTL", "module warn"));
}
