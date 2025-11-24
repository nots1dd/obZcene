#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "Math/simd/x86/trig_pack.h"
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
    printf("SIN/COS/TAN SIMD Test: %s\n", MHM_SIMD_DESC);

    const int N = 2000000;
    const int RUNS = 2;

    float *xs  = malloc(sizeof(float) * N);
    float *out = malloc(sizeof(float) * N);

    for (int i = 0; i < N; i++)
        xs[i] = frand(-10.0f, 10.0f); // typical trig range

    // scalar references
    double t0 = mhm_now_s();
    for (int r = 0; r < RUNS; r++)
        for (int i = 0; i < N; i++)
            out[i] = sinf(xs[i]);
    double t_sin = mhm_now_s() - t0;
    printf("scalar sinf: time=%.6f\n", t_sin);

    t0 = mhm_now_s();
    for (int r = 0; r < RUNS; r++)
        for (int i = 0; i < N; i++)
            out[i] = cosf(xs[i]);
    double t_cos = mhm_now_s() - t0;
    printf("scalar cosf: time=%.6f\n", t_cos);

    t0 = mhm_now_s();
    for (int r = 0; r < RUNS; r++)
        for (int i = 0; i < N; i++)
            out[i] = tanf(xs[i]);
    double t_tan = mhm_now_s() - t0;
    printf("scalar tanf: time=%.6f\n\n", t_tan);

    // ---------------- SIMD tests ----------------
#if MHM_HAS_SSE
    BENCH_SIMD_TYPE(float, sinf, __SIMD_sin_vec_sse, (xs, out, N), "SSE sin", RUNS, xs, out, N, t_sin);
    BENCH_SIMD_TYPE(float, cosf, __SIMD_cos_vec_sse, (xs, out, N), "SSE cos", RUNS, xs, out, N, t_cos);
    BENCH_SIMD_TYPE(float, tanf, __SIMD_tan_vec_sse, (xs, out, N), "SSE tan", RUNS, xs, out, N, t_tan);
#endif

#if MHM_HAS_AVX2
    BENCH_SIMD_TYPE(float, sinf, __SIMD_sin_vec_avx2, (xs, out, N), "AVX2 sin", RUNS, xs, out, N, t_sin);
    BENCH_SIMD_TYPE(float, cosf, __SIMD_cos_vec_avx2, (xs, out, N), "AVX2 cos", RUNS, xs, out, N, t_cos);
    BENCH_SIMD_TYPE(float, tanf, __SIMD_tan_vec_avx2, (xs, out, N), "AVX2 tan", RUNS, xs, out, N, t_tan);
#endif

#if MHM_HAS_AVX512
    BENCH_SIMD_TYPE(float, sinf, __SIMD_sin_vec_avx512, (xs, out, N), "AVX-512 sin", RUNS, xs, out, N, t_sin);
    BENCH_SIMD_TYPE(float, cosf, __SIMD_cos_vec_avx512, (xs, out, N), "AVX-512 cos", RUNS, xs, out, N, t_cos);
    BENCH_SIMD_TYPE(float, tanf, __SIMD_tan_vec_avx512, (xs, out, N), "AVX-512 tan", RUNS, xs, out, N, t_tan);
#endif

    free(xs);
    free(out);
    return 0;
}
