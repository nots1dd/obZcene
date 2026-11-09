#pragma once

/**
 * @file Pack4.hpp
 * @brief A 4-wide float pack with SSE2 and scalar backends (obzcene::math).
 *
 * The rasterizer's inner loop uses Pack4f to evaluate four pixels per
 * iteration. The API is deliberately tiny — splat/iota/load/store,
 * componentwise arithmetic and mask comparisons — and every operation
 * applies the same IEEE-754 operation per lane on both backends, so results
 * are bit-identical whether or not SSE2 is available.
 */

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace obzcene::math
{

/**
 * @brief Four floats processed as one unit.
 *
 * On SSE2 builds this wraps `__m128`; otherwise it is a plain float[4] with
 * the same operations spelled out per lane. Comparisons produce mask packs
 * (-1.0F for true, 0.0F for false); combine them with `operator&` and test
 * them with #movemask.
 */
struct Pack4f
{
#if defined(__SSE2__)
  __m128 lanes;

  /** @brief Wrap an existing register value (see the arithmetic operators). */
  explicit Pack4f(__m128 v) : lanes(v) {}
#else
  float lanes[4];
#endif

  /** @brief Broadcast one scalar into all four lanes. */
  explicit Pack4f(float s)
#if defined(__SSE2__)
      : lanes(_mm_set1_ps(s)){}
#else
      : lanes{s, s, s, s}
  {
  }
#endif

        /** @brief The sequence {0, 1, 2, 3} — seeds per-lane offsets. */
        [[nodiscard]] static auto iota() -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_setr_ps(0.0F, 1.0F, 2.0F, 3.0F)};
#else
    return Pack4f{0.0F, 1.0F, 2.0F, 3.0F};
#endif
  }

  /** @brief Unaligned load of four consecutive floats. */
  [[nodiscard]] static auto load(const float* p) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_loadu_ps(p)};
#else
    return Pack4f{p[0], p[1], p[2], p[3]};
#endif
  }

  /** @brief Unaligned store of the four lanes to `p`. */
  auto store(float* p) const -> void
  {
#if defined(__SSE2__)
    _mm_storeu_ps(p, lanes);
#else
    p[0] = lanes[0];
    p[1] = lanes[1];
    p[2] = lanes[2];
    p[3] = lanes[3];
#endif
  }

  /** @brief Bit i of the result is set when lane i of this mask pack is true. */
  [[nodiscard]] auto movemask() const -> int
  {
#if defined(__SSE2__)
    return _mm_movemask_ps(lanes);
#else
    auto mask = 0;
    for (auto i = 0; i < 4; ++i)
    {
      if (lanes[i] != 0.0F)
      {
        mask |= 1 << i;
      }
    }
    return mask;
#endif
  }

  friend auto operator+(const Pack4f& a, const Pack4f& b) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_add_ps(a.lanes, b.lanes)};
#else
    return Pack4f{a.lanes[0] + b.lanes[0], a.lanes[1] + b.lanes[1], a.lanes[2] + b.lanes[2],
                  a.lanes[3] + b.lanes[3]};
#endif
  }

  friend auto operator*(const Pack4f& a, const Pack4f& b) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_mul_ps(a.lanes, b.lanes)};
#else
    return Pack4f{a.lanes[0] * b.lanes[0], a.lanes[1] * b.lanes[1], a.lanes[2] * b.lanes[2],
                  a.lanes[3] * b.lanes[3]};
#endif
  }

  friend auto operator/(const Pack4f& a, const Pack4f& b) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_div_ps(a.lanes, b.lanes)};
#else
    return Pack4f{a.lanes[0] / b.lanes[0], a.lanes[1] / b.lanes[1], a.lanes[2] / b.lanes[2],
                  a.lanes[3] / b.lanes[3]};
#endif
  }

  /** @brief Componentwise `a >= b` as a mask pack (-1.0F true, 0.0F false). */
  friend auto operator>=(const Pack4f& a, const Pack4f& b) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_cmpge_ps(a.lanes, b.lanes)};
#else
    return Pack4f{a.lanes[0] >= b.lanes[0] ? -1.0F : 0.0F, a.lanes[1] >= b.lanes[1] ? -1.0F : 0.0F,
                  a.lanes[2] >= b.lanes[2] ? -1.0F : 0.0F, a.lanes[3] >= b.lanes[3] ? -1.0F : 0.0F};
#endif
  }

  /** @brief Componentwise `a < b` as a mask pack (-1.0F true, 0.0F false). */
  friend auto operator<(const Pack4f& a, const Pack4f& b) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_cmplt_ps(a.lanes, b.lanes)};
#else
    return Pack4f{a.lanes[0] < b.lanes[0] ? -1.0F : 0.0F, a.lanes[1] < b.lanes[1] ? -1.0F : 0.0F,
                  a.lanes[2] < b.lanes[2] ? -1.0F : 0.0F, a.lanes[3] < b.lanes[3] ? -1.0F : 0.0F};
#endif
  }

  /** @brief Logical AND of two mask packs. */
  friend auto operator&(const Pack4f& a, const Pack4f& b) -> Pack4f
  {
#if defined(__SSE2__)
    return Pack4f{_mm_and_ps(a.lanes, b.lanes)};
#else
    const auto pick = [](float x, float y) -> float
    { return (x != 0.0F && y != 0.0F) ? -1.0F : 0.0F; };
    return Pack4f{pick(a.lanes[0], b.lanes[0]), pick(a.lanes[1], b.lanes[1]),
                  pick(a.lanes[2], b.lanes[2]), pick(a.lanes[3], b.lanes[3])};
#endif
  }

private:
#if !defined(__SSE2__)
  /** @brief Componentwise constructor (scalar backend only). */
  Pack4f(float x0, float x1, float x2, float x3) : lanes{x0, x1, x2, x3} {}
#endif
};

} // namespace obzcene::math
