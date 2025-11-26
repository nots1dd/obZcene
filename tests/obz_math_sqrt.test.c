#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <immintrin.h>

#include "Math/simd/x86/sqrt.h"
#include "test.h"

// ---------------- RNG ----------------
static uint32_t rng_state = 0x12345678u;
static inline uint32_t xrng32(void) {
    uint32_t x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rng_state = x;
}
static inline float frand(float a, float b) {
    float t = (xrng32() & 0xFFFFFF) / (float)0x1000000;
    return a + t * (b - a);
}

// ---------------- Main benchmark ----------------
int main(void)
{
    printf("SQRT & RSQRT SIMD Test: %s\n", OBZ_SIMD_DESC);

    const int N = 2000000;
    const int RUNS = 2;

    float *xs  = malloc(sizeof(float) * N);
    float *out = malloc(sizeof(float) * N);

    for (int i = 0; i < N; i++)
        xs[i] = frand(0.001f, 10000.0f);

    // scalar reference
    double t0 = OBZ_now_s();
    for (int r = 0; r < RUNS; r++)
        for (int i = 0; i < N; i++)
            out[i] = sqrtf(xs[i]);
    double t_sqrtf = OBZ_now_s() - t0;
    printf("scalar sqrtf: time=%.6f\n", t_sqrtf);
    
    t0 = OBZ_now_s();
    for (int r = 0; r < RUNS; r++)
        for (int i = 0; i < N; i++)
            out[i] = rsqrtf(xs[i]);
    double t_rsqrtf = OBZ_now_s() - t0;
    printf("scalar rsqrtf: time=%.6f\n\n", t_rsqrtf);

    // SSE sqrt
#if OBZ_HAS_SSE
    BENCH_SIMD_TYPE(float, sqrtf, __SIMD_sqrt_vec_sse, (xs, out, N), "SSE sqrt", RUNS, xs, out, N, t_sqrtf);
    BENCH_SIMD_TYPE(float, rsqrtf, __SIMD_rsqrt_vec_sse, (xs, out, N), "SSE rsqrt", RUNS, xs, out, N, t_rsqrtf);
#endif

#if OBZ_HAS_AVX2
    BENCH_SIMD_TYPE(float, sqrtf, __SIMD_sqrt_vec_avx2, (xs, out, N), "AVX2 sqrt", RUNS, xs, out, N, t_sqrtf);
    BENCH_SIMD_TYPE(float, rsqrtf, __SIMD_rsqrt_newton_vec_avx2, (xs, out, N), "AVX2 rsqrt Newton", RUNS, xs, out, N, t_rsqrtf);
#endif

#if OBZ_HAS_AVX512
    BENCH_SIMD_TYPE(float, sqrtf, __SIMD_sqrt_vec_avx512, (xs, out, N), "AVX-512 sqrt", RUNS, xs, out, N, t_sqrtf);
    BENCH_SIMD_TYPE(float, rsqrtf, __SIMD_rsqrt_newton_vec_avx512, (xs, out, N), "AVX-512 rsqrt Newton", RUNS, xs, out, N, t_rsqrtf);
#endif

    free(xs);
    free(out);
    return 0;
}
