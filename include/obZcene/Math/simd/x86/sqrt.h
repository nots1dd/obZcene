#ifndef OBZ_MATH_SIMD_X86_SQRT_H
#define OBZ_MATH_SIMD_X86_SQRT_H

#include "intrin/sqrt.h"
#include "obz_macros.h"

OBZ_BEGIN_CPP_DECLS

OBZ_API_INLINE static float obz_sqrtf(float x)
{
#if OBZ_HAS_SSE
  return _OBZ__SIMD_sqrt_scalar_sse(x);
#elif OBZ_HAS_AVX2
  return _OBZ__SIMD_sqrt_scalar_avx2(x);
#elif OBZ_HAS_AVX512
  return _OBZ__SIMD_sqrt_scalar_avx512(x);
#else
  return sqrtf(x);
#endif
}

OBZ_API_INLINE static float obz_rsqrtf(float x)
{
#if OBZ_HAS_SSE
  return _OBZ__SIMD_rsqrt_scalar_sse(x);
#elif OBZ_HAS_AVX2
  return _OBZ__SIMD_rsqrt_scalar_avx2(x);
#elif OBZ_HAS_AVX512
  return _OBZ__SIMD_rsqrt_scalar_avx512(x);
#else
  return rsqrtf(x);
#endif
}

OBZ_END_CPP_DECLS

#endif
