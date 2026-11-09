#pragma once

/**
 * @file Logger.hpp
 * @brief Thin wrapper around spdlog plus convenience logging macros.
 *
 * `Logger::init()` must run before any logging occurs; `entry()` in
 * src/App/App.cpp calls it for both the interactive viewer and snapshot mode.
 * Module-specific loggers are created lazily by name (e.g. the OBJ/MTL parsers
 * use `LOG_MODULE_*`), while the generic macros always target the "CORE"
 * logger.
 *
 * @see src/Core/Logger.cpp (spdlog setup),
 *      File/Obj/Parser/ObjParser.hpp / File/Mtl/Parser/MtlParser.hpp (module
 *      loggers)
 */

#include <memory>
#include <string>

#include <spdlog/spdlog.h>

namespace obzcene
{

/**
 * @brief Static facade over the spdlog core logger.
 */
class Logger
{
public:
  /** @brief Create the core logger and set the global format pattern. */
  static auto init() -> void;
  /** @brief Flush and release all spdlog resources. */
  static auto shutdown() -> void;

  /** @brief The core logger (may be used directly, e.g. `LOG_INFO`). */
  [[nodiscard]] static auto get() -> std::shared_ptr<spdlog::logger>&;
  /**
   * @brief A lazily-created logger under the given `name`.
   * @param name Logger name; repeated calls return the same instance, so
   *             "OBJ"/"MTL" loggers are stable across a process run.
   */
  [[nodiscard]] static auto get(const std::string& name) -> std::shared_ptr<spdlog::logger>;

private:
  static std::shared_ptr<spdlog::logger> s_core_logger;
};

} // namespace obzcene

/** @brief Log at TRACE level on the core logger. */
#define LOG_TRACE(...) ::obzcene::Logger::get()->trace(__VA_ARGS__)
/** @brief Log at INFO level on the core logger. */
#define LOG_INFO(...) ::obzcene::Logger::get()->info(__VA_ARGS__)
/** @brief Log at WARN level on the core logger. */
#define LOG_WARN(...) ::obzcene::Logger::get()->warn(__VA_ARGS__)
/** @brief Log at ERROR level on the core logger. */
#define LOG_ERROR(...) ::obzcene::Logger::get()->error(__VA_ARGS__)
/** @brief Log at CRITICAL level on the core logger. */
#define LOG_CRITICAL(...) ::obzcene::Logger::get()->critical(__VA_ARGS__)

/** @brief Log at TRACE level on the logger named `name`. */
#define LOG_MODULE_TRACE(name, ...) ::obzcene::Logger::get(name)->trace(__VA_ARGS__)
/** @brief Log at INFO level on the logger named `name`. */
#define LOG_MODULE_INFO(name, ...) ::obzcene::Logger::get(name)->info(__VA_ARGS__)
/** @brief Log at WARN level on the logger named `name`. */
#define LOG_MODULE_WARN(name, ...) ::obzcene::Logger::get(name)->warn(__VA_ARGS__)
/** @brief Log at ERROR level on the logger named `name`. */
#define LOG_MODULE_ERROR(name, ...) ::obzcene::Logger::get(name)->error(__VA_ARGS__)
/** @brief Log at CRITICAL level on the logger named `name`. */
#define LOG_MODULE_CRITICAL(name, ...) ::obzcene::Logger::get(name)->critical(__VA_ARGS__)
