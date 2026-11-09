#pragma once

/**
 * @file Timer.hpp
 * @brief Time helpers under `obzcene::utils::timer` (currently one timestamp printer).
 *
 * Used to stamp the debug backtrace events; kept self-contained on
 * `<chrono>`/`<ctime>` so it never drags the whole engine in.
 *
 * @see Backtrace.hpp (Event uses timestamp_now)
 */

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace obzcene::utils::timer
{

/**
 * @brief Current local wall-clock time with millisecond precision.
 * @return e.g. `2026-09-12 14:03:07.421` (local timezone).
 */
inline auto timestamp_now() -> std::string
{
  auto now  = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0')
      << std::setw(3) << ms.count();
  return oss.str();
}

} // namespace obzcene::utils::timer
