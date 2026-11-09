/**
 * @file Logger.cpp
 * @brief spdlog setup: core logger, global pattern, named sub-loggers.
 *
 * The pattern prints a colourised `[time] name: message`. `get(name)` reuses
 * or lazily creates the named logger (all at TRACE level), which is how the
 * independent OBJ/MTL parser logs coexist with the CORE stream.
 *
 * @see Core/Logger.hpp (facade + macros); the OBJ/MTL parsers register their
 *      own module loggers with get()
 */

#include "obzcene/Core/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace
{

/**
 * @brief Return the named logger, creating it on first use.
 * @param name  Logger name; the first call registers a colour stdout logger.
 */
inline auto ensure_named_logger(const std::string& name) -> std::shared_ptr<spdlog::logger>
{
  if (auto existing = spdlog::get(name))
  {
    return existing;
  }
  auto logger = spdlog::stdout_color_mt(name);
  logger->set_level(spdlog::level::trace);
  return logger;
}

} // namespace

namespace obzcene
{

std::shared_ptr<spdlog::logger> Logger::s_core_logger;

auto Logger::init() -> void
{
  spdlog::set_pattern("%^[%T] %n: %v%$");
  s_core_logger = ensure_named_logger("CORE");
}

auto Logger::shutdown() -> void { spdlog::shutdown(); }

auto Logger::get() -> std::shared_ptr<spdlog::logger>& { return s_core_logger; }

auto Logger::get(const std::string& name) -> std::shared_ptr<spdlog::logger>
{
  return ensure_named_logger(name);
}

} // namespace obzcene
