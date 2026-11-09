#pragma once

/**
 * @file Casts.hpp
 * @brief Integer cast that preserves value semantics (never truncates in the
 *        value-preserving cases it is told about).
 *
 * Replaces implicit/dangerous integer conversions across the codebase,
 * e.g. `std::size_t` -> `ui32` index obligations in
 * obzcene::build_mesh. It relies on `std::cmp_*` comparisons that are correct
 * across signed/unsigned mixes, and only asserts where a value could actually
 * be lost.
 *
 * @see Renderer/Mesh.hpp (narrow_cast usages while building vertices),
 *      Viewer/Overlay.cpp (narrowing in text metrics)
 */

#include <cassert>
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

namespace obzcene
{

/**
 * @brief Cast between integral types, asserting the value fits `To`.
 * @tparam To Target integral type.
 * @tparam From Source integral type (deduced).
 * @param value Integral value to convert.
 * @return The value converted via `static_cast`.
 *
 * @note When the sign matches and `To` is at least as wide as `From` the cast
 *       is guaranteed to fit and skips the check entirely; otherwise a debug
 *       assert verifies the value is within [lowest, max] of `To`.
 */
template <std::integral To, std::integral From>
[[nodiscard]] constexpr auto narrow_cast(const From value) noexcept -> To
{
  constexpr auto guaranteed_fit =
    std::is_signed_v<To> == std::is_signed_v<From> && sizeof(To) >= sizeof(From);
  if constexpr (guaranteed_fit)
  {
    return static_cast<To>(value);
  }
  assert(std::cmp_greater_equal(value, std::numeric_limits<To>::lowest()) &&
         std::cmp_less_equal(value, std::numeric_limits<To>::max()));
  return static_cast<To>(value);
}

} // namespace obzcene