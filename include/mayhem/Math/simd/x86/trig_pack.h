#ifndef MHM_MATH_SIMD_X86_TRIG_H
#define MHM_MATH_SIMD_X86_TRIG_H

#include "config.h"
#ifndef MHM_CONFIG_H
# error "Include MHM_CONFIG_H before this header"
#endif
#if !(MHM_ARCH_X86_64 || MHM_ARCH_X86_32)
# error "This header supports x86 targets only"
#endif
#include <immintrin.h>

/* =========================================================================
   Constants
   ========================================================================= */
#define M_PI_F 3.14159265358979323846f
#define M_PI_2_F 1.57079632679489661923f
#define M_2PI_F 6.28318530717958647692f

/* =========================================================================
   Range reduction for [-pi/2, pi/2]
   ========================================================================= */
static inline __m128 __range_reduce_ps_sse(__m128 x) {
    // x -> [-pi, pi]
    __m128 k = _mm_round_ps(_mm_div_ps(x, _mm_set1_ps(M_2PI_F)), _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    __m128 xr = _mm_sub_ps(x, _mm_mul_ps(k, _mm_set1_ps(M_2PI_F)));
    // map to [-pi/2, pi/2]
    __m128 mask = _mm_cmpgt_ps(xr, _mm_set1_ps(M_PI_2_F));
    xr = _mm_blendv_ps(xr, _mm_sub_ps(_mm_set1_ps(M_PI_F), xr), mask);
    mask = _mm_cmplt_ps(xr, _mm_set1_ps(-M_PI_2_F));
    xr = _mm_blendv_ps(xr, _mm_sub_ps(-_mm_set1_ps(M_PI_F), xr), mask);
    return xr;
}

#if MHM_HAS_AVX2
static inline __m256 __range_reduce_ps_avx2(__m256 x) {
    __m256 k = _mm256_round_ps(_mm256_div_ps(x, _mm256_set1_ps(M_2PI_F)), _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    __m256 xr = _mm256_sub_ps(x, _mm256_mul_ps(k, _mm256_set1_ps(M_2PI_F)));
    __m256 mask = _mm256_cmp_ps(xr, _mm256_set1_ps(M_PI_2_F), _CMP_GT_OS);
    xr = _mm256_blendv_ps(xr, _mm256_sub_ps(_mm256_set1_ps(M_PI_F), xr), mask);
    mask = _mm256_cmp_ps(xr, _mm256_set1_ps(-M_PI_2_F), _CMP_LT_OS);
    xr = _mm256_blendv_ps(xr, _mm256_sub_ps(-_mm256_set1_ps(M_PI_F), xr), mask);
    return xr;
}
#endif

#if MHM_HAS_AVX512
static inline __m512 __range_reduce_ps_avx512(__m512 x) {
    __m512 k = _mm512_roundscale_ps(x, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    k = _mm512_div_ps(x, _mm512_set1_ps(M_2PI_F));
    __m512 xr = _mm512_sub_ps(x, _mm512_mul_ps(k, _mm512_set1_ps(M_2PI_F)));
    __mmask16 mask = _mm512_cmp_ps_mask(xr, _mm512_set1_ps(M_PI_2_F), _CMP_GT_OS);
    xr = _mm512_mask_blend_ps(mask, xr, _mm512_sub_ps(_mm512_set1_ps(M_PI_F), xr));
    mask = _mm512_cmp_ps_mask(xr, _mm512_set1_ps(-M_PI_2_F), _CMP_LT_OS);
    xr = _mm512_mask_blend_ps(mask, xr, _mm512_sub_ps(-_mm512_set1_ps(M_PI_F), xr));
    return xr;
}
#endif

/* =========================================================================
   Polynomial approximations (minimax)
   sin(x) ~ x*(1 - x^2/6 + x^4/120 - x^6/5040 + ...)
   cos(x) ~ 1 - x^2/2 + x^4/24 - x^6/720 + ...
   ========================================================================= */

/* SSE */
static inline __m128 __SIMD_sin_ps_sse(__m128 x) {
    x = __range_reduce_ps_sse(x);
    __m128 x2 = _mm_mul_ps(x, x);
    __m128 t;

    t = _mm_set1_ps(-2.39e-08f);
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(2.7526e-06f));
    t = _mm_sub_ps(_mm_mul_ps(t, x2), _mm_set1_ps(1.98409e-04f));
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(8.3333315e-03f));
    t = _mm_sub_ps(_mm_mul_ps(t, x2), _mm_set1_ps(1.666666664e-01f));
    t = _mm_mul_ps(t, x2);
    t = _mm_add_ps(t, x);
    return t;
}

static inline __m128 __SIMD_cos_ps_sse(__m128 x) {
    x = __range_reduce_ps_sse(x);
    __m128 x2 = _mm_mul_ps(x, x);
    __m128 t;

    t = _mm_set1_ps(-2.605e-07f);
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(2.47609e-05f));
    t = _mm_sub_ps(_mm_mul_ps(t, x2), _mm_set1_ps(1.3888397e-03f));
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(4.16666418e-02f));
    t = _mm_sub_ps(_mm_mul_ps(t, x2), _mm_set1_ps(0.5f));
    t = _mm_add_ps(t, _mm_set1_ps(1.0f));
    return t;
}

static inline __m128 __SIMD_tan_ps_sse(__m128 x) {
    x = __range_reduce_ps_sse(x);
    __m128 x2 = _mm_mul_ps(x, x);
    __m128 t;

    t = _mm_set1_ps(62.0f/2835.0f);
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(17.0f/315.0f));
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(2.0f/15.0f));
    t = _mm_add_ps(_mm_mul_ps(t, x2), _mm_set1_ps(1.0f/3.0f));
    t = _mm_mul_ps(t, _mm_mul_ps(x, x2)); // t * x^3
    t = _mm_add_ps(t, x);                 // add linear term
    return t;
}

/* AVX2 */
#if MHM_HAS_AVX2
static inline __m256 __SIMD_sin_ps_avx2(__m256 x) {
    x = __range_reduce_ps_avx2(x);
    __m256 x2 = _mm256_mul_ps(x, x);
    __m256 t;

    t = _mm256_set1_ps(-2.39e-08f);
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(2.7526e-06f));
    t = _mm256_sub_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(1.98409e-04f));
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(8.3333315e-03f));
    t = _mm256_sub_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(1.666666664e-01f));
    t = _mm256_mul_ps(t, x2);
    t = _mm256_add_ps(t, x);
    return t;
}

static inline __m256 __SIMD_cos_ps_avx2(__m256 x) {
    x = __range_reduce_ps_avx2(x);
    __m256 x2 = _mm256_mul_ps(x, x);
    __m256 t;

    t = _mm256_set1_ps(-2.605e-07f);
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(2.47609e-05f));
    t = _mm256_sub_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(1.3888397e-03f));
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(4.16666418e-02f));
    t = _mm256_sub_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(0.5f));
    t = _mm256_add_ps(t, _mm256_set1_ps(1.0f));
    return t;
}

static inline __m256 __SIMD_tan_ps_avx2(__m256 x) {
    x = __range_reduce_ps_avx2(x);
    __m256 x2 = _mm256_mul_ps(x, x);
    __m256 t;

    t = _mm256_set1_ps(62.0f/2835.0f);
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(17.0f/315.0f));
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(2.0f/15.0f));
    t = _mm256_add_ps(_mm256_mul_ps(t, x2), _mm256_set1_ps(1.0f/3.0f));
    t = _mm256_mul_ps(t, _mm256_mul_ps(x, x2));
    t = _mm256_add_ps(t, x);
    return t;
}
#endif

/* AVX-512 */
#if MHM_HAS_AVX512
static inline __m512 __SIMD_sin_ps_avx512(__m512 x) {
    x = __range_reduce_ps_avx512(x);
    __m512 x2 = _mm512_mul_ps(x, x);
    __m512 t;

    t = _mm512_set1_ps(-2.39e-08f);
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(2.7526e-06f));
    t = _mm512_sub_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(1.98409e-04f));
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(8.3333315e-03f));
    t = _mm512_sub_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(1.666666664e-01f));
    t = _mm512_mul_ps(t, x2);
    t = _mm512_add_ps(t, x);
    return t;
}

static inline __m512 __SIMD_cos_ps_avx512(__m512 x) {
    x = __range_reduce_ps_avx512(x);
    __m512 x2 = _mm512_mul_ps(x, x);
    __m512 t;

    t = _mm512_set1_ps(-2.605e-07f);
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(2.47609e-05f));
    t = _mm512_sub_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(1.3888397e-03f));
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(4.16666418e-02f));
    t = _mm512_sub_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(0.5f));
    t = _mm512_add_ps(t, _mm512_set1_ps(1.0f));
    return t;
}

static inline __m512 __SIMD_tan_ps_avx512(__m512 x) {
    x = __range_reduce_ps_avx512(x);
    __m512 x2 = _mm512_mul_ps(x, x);
    __m512 t;

    t = _mm512_set1_ps(62.0f/2835.0f);
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(17.0f/315.0f));
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(2.0f/15.0f));
    t = _mm512_add_ps(_mm512_mul_ps(t, x2), _mm512_set1_ps(1.0f/3.0f));
    t = _mm512_mul_ps(t, _mm512_mul_ps(x, x2));
    t = _mm512_add_ps(t, x);
    return t;
}
#endif

/* =========================================================================
   Scalar wrappers
   ========================================================================= */
static inline float __SIMD_sin_scalar_sse(float x) {
    __m128 v = _mm_set_ss(x);
    return _mm_cvtss_f32(__SIMD_sin_ps_sse(v));
}

static inline float __SIMD_cos_scalar_sse(float x) {
    __m128 v = _mm_set_ss(x);
    return _mm_cvtss_f32(__SIMD_cos_ps_sse(v));
}

static inline float __SIMD_tan_scalar_sse(float x) {
    __m128 v = _mm_set_ss(x);
    return _mm_cvtss_f32(__SIMD_tan_ps_sse(v));
}

#if MHM_HAS_AVX2
static inline float __SIMD_sin_scalar_avx2(float x) {
    __m256 v = _mm256_set1_ps(x);
    return _mm256_cvtss_f32(__SIMD_sin_ps_avx2(v));
}

static inline float __SIMD_cos_scalar_avx2(float x) {
    __m256 v = _mm256_set1_ps(x);
    return _mm256_cvtss_f32(__SIMD_cos_ps_avx2(v));
}

static inline float __SIMD_tan_scalar_avx2(float x) {
    __m256 v = _mm256_set1_ps(x);
    return _mm256_cvtss_f32(__SIMD_tan_ps_avx2(v));
}
#endif

#if MHM_HAS_AVX512
static inline float __SIMD_sin_scalar_avx512(float x) {
    __m512 v = _mm512_set1_ps(x);
    return _mm512_cvtss_f32(__SIMD_sin_ps_avx512(v));
}

static inline float __SIMD_cos_scalar_avx512(float x) {
    __m512 v = _mm512_set1_ps(x);
    return _mm512_cvtss_f32(__SIMD_cos_ps_avx512(v));
}

static inline float __SIMD_tan_scalar_avx512(float x) {
    __m512 v = _mm512_set1_ps(x);
    return _mm512_cvtss_f32(__SIMD_tan_ps_avx512(v));
}
#endif

/* =========================================================================
   Vectorized wrappers for arrays
   ========================================================================= */

/* SSE */
static inline void __SIMD_sin_vec_sse(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-4; i+=4) {
        __m128 v = _mm_loadu_ps(xs + i);
        _mm_storeu_ps(out + i, __SIMD_sin_ps_sse(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_sin_scalar_sse(xs[i]);
}

static inline void __SIMD_cos_vec_sse(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-4; i+=4) {
        __m128 v = _mm_loadu_ps(xs + i);
        _mm_storeu_ps(out + i, __SIMD_cos_ps_sse(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_cos_scalar_sse(xs[i]);
}

static inline void __SIMD_tan_vec_sse(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-4; i+=4) {
        __m128 v = _mm_loadu_ps(xs + i);
        _mm_storeu_ps(out + i, __SIMD_tan_ps_sse(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_tan_scalar_sse(xs[i]);
}

#if MHM_HAS_AVX2
/* AVX2 */
static inline void __SIMD_sin_vec_avx2(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-8; i+=8) {
        __m256 v = _mm256_loadu_ps(xs + i);
        _mm256_storeu_ps(out + i, __SIMD_sin_ps_avx2(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_sin_scalar_avx2(xs[i]);
}

static inline void __SIMD_cos_vec_avx2(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-8; i+=8) {
        __m256 v = _mm256_loadu_ps(xs + i);
        _mm256_storeu_ps(out + i, __SIMD_cos_ps_avx2(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_cos_scalar_avx2(xs[i]);
}

static inline void __SIMD_tan_vec_avx2(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-8; i+=8) {
        __m256 v = _mm256_loadu_ps(xs + i);
        _mm256_storeu_ps(out + i, __SIMD_tan_ps_avx2(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_tan_scalar_avx2(xs[i]);
}
#endif

#if MHM_HAS_AVX512
/* AVX-512 */
static inline void __SIMD_sin_vec_avx512(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-16; i+=16) {
        __m512 v = _mm512_loadu_ps(xs + i);
        _mm512_storeu_ps(out + i, __SIMD_sin_ps_avx512(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_sin_scalar_avx512(xs[i]);
}

static inline void __SIMD_cos_vec_avx512(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-16; i+=16) {
        __m512 v = _mm512_loadu_ps(xs + i);
        _mm512_storeu_ps(out + i, __SIMD_cos_ps_avx512(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_cos_scalar_avx512(xs[i]);
}

static inline void __SIMD_tan_vec_avx512(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-16; i+=16) {
        __m512 v = _mm512_loadu_ps(xs + i);
        _mm512_storeu_ps(out + i, __SIMD_tan_ps_avx512(v));
    }
    for (; i<N; i++)
        out[i] = __SIMD_tan_scalar_avx512(xs[i]);
}
#endif

#endif /* MHM_MATH_SIMD_X86_TRIG_H */
