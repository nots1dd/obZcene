#ifndef OBZ_MATH_SIMD_X86_SQRT_H
#define OBZ_MATH_SIMD_X86_SQRT_H

#include "config.h"

#ifndef OBZ_CONFIG_H
# error "Include OBZ_CONFIG_H before this header"
#endif

#if !(OBZ_ARCH_X86_64 || OBZ_ARCH_X86_32)
# error "This header supports x86 targets only"
#endif

#include <immintrin.h>

/* =========================================================================
   SIMD Square Root / Reciprocal Square Root (SSE, AVX2, AVX-512)
   - SSE 128-bit
   - AVX2 256-bit
   - AVX-512 512-bit
   Stable API format: __SIMD_<func>
   ========================================================================= */

/* -------------------- SSE (128-bit) -------------------- */

/**
 * __SIMD_sqrt_ps_sse:
 * Computes sqrt of each float in __m128 vector.
 */
static inline __m128 __SIMD_sqrt_ps_sse(__m128 x) {
    return _mm_sqrt_ps(x);
}

/**
 * __SIMD_rsqrt_ps_sse:
 * Computes approximate reciprocal sqrt of each float in __m128 vector.
 * Fast (~12-bit precision).
 */
static inline __m128 __SIMD_rsqrt_ps_sse(__m128 x) {
    return _mm_rsqrt_ps(x);
}

/**
 * __SIMD_rsqrt_ps_newton_sse:
 * Computes reciprocal sqrt using one Newton-Raphson iteration on SSE vector.
 * Improves accuracy over approximate rsqrt.
 */
static inline __m128 __SIMD_rsqrt_ps_newton_sse(__m128 x) {
    __m128 y = _mm_rsqrt_ps(x);
    __m128 half = _mm_set1_ps(0.5f);
    __m128 three = _mm_set1_ps(3.0f);
    return _mm_mul_ps(y, _mm_mul_ps(half,
                _mm_sub_ps(three, _mm_mul_ps(x, _mm_mul_ps(y, y)))));
}

/* -------------------- AVX2 (256-bit) -------------------- */
#if OBZ_HAS_AVX2

static inline __m256 __SIMD_sqrt_ps_avx2(__m256 x) {
    return _mm256_sqrt_ps(x);
}

static inline __m256 __SIMD_rsqrt_ps_avx2(__m256 x) {
    return _mm256_rsqrt_ps(x); // Approximate
}

static inline __m256 __SIMD_rsqrt_ps_newton_avx2(__m256 x) {
    __m256 y = _mm256_rsqrt_ps(x);
    __m256 half = _mm256_set1_ps(0.5f);
    __m256 three = _mm256_set1_ps(3.0f);
    return _mm256_mul_ps(y, _mm256_mul_ps(half,
                _mm256_sub_ps(three, _mm256_mul_ps(x, _mm256_mul_ps(y, y)))));
}

#endif /* OBZ_HAS_AVX2 */

/* -------------------- AVX-512 (512-bit) -------------------- */
#if OBZ_HAS_AVX512

static inline __m512 __SIMD_sqrt_ps_avx512(__m512 x) {
    return _mm512_sqrt_ps(x);
}

static inline __m512 __SIMD_rsqrt_ps_avx512(__m512 x) {
    __m512 one = _mm512_set1_ps(1.0f);
    return _mm512_div_ps(one, _mm512_sqrt_ps(x)); // precise
}

static inline __m512 __SIMD_rsqrt_ps_newton_avx512(__m512 x) {
    __m512 y = __SIMD_rsqrt_ps_avx512(x);
    __m512 half = _mm512_set1_ps(0.5f);
    __m512 three = _mm512_set1_ps(3.0f);
    return _mm512_mul_ps(y, _mm512_mul_ps(half,
                _mm512_sub_ps(three, _mm512_mul_ps(x, _mm512_mul_ps(y, y)))));
}

#endif /* OBZ_HAS_AVX512 */

/* =========================================================================
   Scalar Wrappers
   Use __SIMD_<func>_scalar format
   ========================================================================= */

/* SSE Scalar */
static inline float __SIMD_sqrt_scalar_sse(float x) {
    __m128 v = _mm_set_ss(x);
    return _mm_cvtss_f32(__SIMD_sqrt_ps_sse(v));
}

static inline float __SIMD_rsqrt_scalar_sse(float x) {
    __m128 v = _mm_set_ss(x);
    return _mm_cvtss_f32(__SIMD_rsqrt_ps_sse(v));
}

static inline float __SIMD_rsqrt_newton_scalar_sse(float x) {
    __m128 v = _mm_set_ss(x);
    return _mm_cvtss_f32(__SIMD_rsqrt_ps_newton_sse(v));
}

/* AVX2 Scalar */
#if OBZ_HAS_AVX2
static inline float __SIMD_sqrt_scalar_avx2(float x) {
    __m256 v = _mm256_set1_ps(x);
    return ((float*)&v)[0];
}

static inline float __SIMD_rsqrt_scalar_avx2(float x) {
    __m256 v = _mm256_set1_ps(x);
    return ((float*)&v)[0];
}

static inline float __SIMD_rsqrt_newton_scalar_avx2(float x) {
    __m256 v = _mm256_set1_ps(x);
    return ((float*)&v)[0];
}
#endif

/* AVX-512 Scalar */
#if OBZ_HAS_AVX512
static inline float __SIMD_sqrt_scalar_avx512(float x) {
    __m512 v = _mm512_set1_ps(x);
    return ((float*)&v)[0];
}

static inline float __SIMD_rsqrt_scalar_avx512(float x) {
    __m512 v = _mm512_set1_ps(x);
    return ((float*)&v)[0];
}

static inline float __SIMD_rsqrt_newton_scalar_avx512(float x) {
    __m512 v = _mm512_set1_ps(x);
    return ((float*)&v)[0];
}
#endif

/* =========================================================================
   Vectorized Wrappers
   Use __SIMD_<func>_vec format
   ========================================================================= */

/* SSE Vector */
static inline void __SIMD_sqrt_vec_sse(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-4; i += 4) {
        __m128 v = _mm_loadu_ps(xs + i);
        __m128 r = __SIMD_sqrt_ps_sse(v);
        _mm_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = __SIMD_sqrt_scalar_sse(xs[i]);
}

static inline void __SIMD_rsqrt_vec_sse(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-4; i += 4) {
        __m128 v = _mm_loadu_ps(xs + i);
        __m128 r = __SIMD_rsqrt_ps_sse(v);
        _mm_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = 1.0f / xs[i];
}

static inline void __SIMD_rsqrt_newton_vec_sse(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-4; i += 4) {
        __m128 v = _mm_loadu_ps(xs + i);
        __m128 r = __SIMD_rsqrt_ps_newton_sse(v);
        _mm_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = 1.0f / xs[i];
}

/* AVX2 Vector */
#if OBZ_HAS_AVX2
static inline void __SIMD_sqrt_vec_avx2(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-8; i += 8) {
        __m256 v = _mm256_loadu_ps(xs + i);
        __m256 r = __SIMD_sqrt_ps_avx2(v);
        _mm256_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = __SIMD_sqrt_scalar_avx2(xs[i]);
}

static inline void __SIMD_rsqrt_vec_avx2(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-8; i += 8) {
        __m256 v = _mm256_loadu_ps(xs + i);
        __m256 r = __SIMD_rsqrt_ps_avx2(v);
        _mm256_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = 1.0f / xs[i];
}

static inline void __SIMD_rsqrt_newton_vec_avx2(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-8; i += 8) {
        __m256 v = _mm256_loadu_ps(xs + i);
        __m256 r = __SIMD_rsqrt_ps_newton_avx2(v);
        _mm256_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = 1.0f / xs[i];
}
#endif

/* AVX-512 Vector */
#if OBZ_HAS_AVX512
static inline void __SIMD_sqrt_vec_avx512(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-16; i += 16) {
        __m512 v = _mm512_loadu_ps(xs + i);
        __m512 r = __SIMD_sqrt_ps_avx512(v);
        _mm512_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = __SIMD_sqrt_scalar_avx512(xs[i]);
}

static inline void __SIMD_rsqrt_vec_avx512(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-16; i += 16) {
        __m512 v = _mm512_loadu_ps(xs + i);
        __m512 r = __SIMD_rsqrt_ps_avx512(v);
        _mm512_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = 1.0f / xs[i];
}

static inline void __SIMD_rsqrt_newton_vec_avx512(const float *xs, float *out, int N) {
    int i;
    for (i = 0; i <= N-16; i += 16) {
        __m512 v = _mm512_loadu_ps(xs + i);
        __m512 r = __SIMD_rsqrt_ps_newton_avx512(v);
        _mm512_storeu_ps(out + i, r);
    }
    for (; i < N; i++) out[i] = 1.0f / xs[i];
}
#endif

#endif /* OBZ_MATH_SIMD_X86_SQRT_H */
