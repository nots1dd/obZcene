#pragma once

/**
 * @file Transforms.hpp
 * @brief Small string utilities under `obzcene::utils::string::transform`.
 *
 * Currently a single ASCII lowercase transform used by the backtrace
 * environment-check in Backtrace.hpp.
 *
 * @see Backtrace.hpp (core::is_stack_trace_enabled)
 */

#include <algorithm>
#include <string>

namespace obzcene::utils::string::transform
{

/**
 * @brief Lowercase every ASCII character of a copy.
 * @return The lowercased string (input is left unmodified).
 */
inline auto tolower_ascii(std::string s) -> std::string
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

} // namespace obzcene::utils::string::transform
