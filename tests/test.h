#ifndef OBZ_TEST_UTILS_H
#define OBZ_TEST_UTILS_H

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>

/* ================================================================
   HIGH-RES TIMER
   ================================================================ */
static inline double OBZ_now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ================================================================
   STATS STRUCT + HELPERS
   Computes:
     - max abs error
     - sum abs error
     - sum squared error
     - relative error (optional)
   ================================================================ */
typedef struct {
    double sum_abs;
    double sum_sq;
    double max_abs;
    double sum_rel;
    long long n;
} OBZ_stats_t;

static inline void OBZ_stats_reset(OBZ_stats_t *s) {
    s->sum_abs = 0;
    s->sum_sq = 0;
    s->sum_rel = 0;
    s->max_abs = 0;
    s->n = 0;
}

static inline void OBZ_stats_add(OBZ_stats_t *s, double ref, double out) {
    double a = fabs(ref - out);
    s->sum_abs += a;
    s->sum_sq += a * a;
    if (a > s->max_abs) s->max_abs = a;

    double r = fabs(ref);
    if (r > 1e-12)
        s->sum_rel += a / r;
    else
        s->sum_rel += 0;

    s->n++;
}

static inline void OBZ_stats_print(const char *label, OBZ_stats_t *s) {
    double N = (double)s->n;
    double mean = s->sum_abs / N;
    double rms  = sqrt(s->sum_sq / N);
    double mean_rel = s->sum_rel / N;

    printf("%s\n", label);
    printf("  max abs error : %.10e\n", s->max_abs);
    printf("  mean abs error: %.10e\n", mean);
    printf("  rms error     : %.10e\n", rms);
    printf("  mean rel error: %.10e\n", mean_rel);
    printf("  samples       : %lld\n\n", s->n);
}

/* ================================================================
   THROUGHPUT HELPERS
   ================================================================ */
static inline double OBZ_mops(double count, double secs) {
    if (secs <= 0) return 0.0;
    return (count / 1e6) / secs;
}

/* ================================================================
   BENCHMARK: SCALAR FUNCTION (call per element)
   Computes:
     - reference timing
     - tested timing
     - speedup
     - throughput
     - full error stats
   ================================================================ */
#define OBZ_BENCH_SCALAR(FREF, FTEST, xs, out, N, RUNS) do {        \
    printf("=== Benchmark: %s vs %s ===\n", #FREF, #FTEST);          \
    double t0, t_ref, t_test;                                        \
    OBZ_stats_t st;                                                  \
                                                                     \
    /* reference */                                                  \
    t0 = OBZ_now_s();                                                \
    for (int r = 0; r < RUNS; r++)                                   \
        for (int i = 0; i < (N); i++)                                \
            out[i] = FREF(xs[i]);                                    \
    t_ref = OBZ_now_s() - t0;                                        \
                                                                     \
    /* test */                                                       \
    t0 = OBZ_now_s();                                                \
    for (int r = 0; r < RUNS; r++)                                   \
        for (int i = 0; i < (N); i++)                                \
            out[i] = FTEST(xs[i]);                                   \
    t_test = OBZ_now_s() - t0;                                       \
                                                                     \
    OBZ_stats_reset(&st);                                            \
    for (int i = 0; i < (N); i++)                                    \
        OBZ_stats_add(&st, FREF(xs[i]), out[i]);                     \
                                                                     \
    printf("time_ref   = %.6f s\n", t_ref);                           \
    printf("time_test  = %.6f s\n", t_test);                          \
    printf("speedup    = %.3fx\n", t_ref / t_test);                   \
    printf("throughput = %.3f Mops/s\n", OBZ_mops((double)N*RUNS, t_test)); \
    printf("\n");                                                    \
    OBZ_stats_print("Error summary:", &st);                          \
} while(0)

/* ================================================================
   BENCHMARK: SIMD (vector batch call)
   FUNC_SIMD(xs, out, N) processes whole array each call.
   Measures:
     - ref time (scalar)
     - SIMD time (array function)
     - speedup
     - throughput
     - errors
   ================================================================ */
#define OBZ_BENCH_SIMD_ARRAY(FREF, FSIMD, xs, out_ref, out_simd, N, RUNS) do { \
    printf("=== SIMD Array Benchmark: %s ===\n", #FSIMD);                       \
                                                                                \
    double t0, t_ref, t_simd;                                                   \
    OBZ_stats_t st;                                                             \
                                                                                \
    /* run reference */                                                         \
    t0 = OBZ_now_s();                                                            \
    for (int r = 0; r < RUNS; r++)                                               \
        for (int i = 0; i < (N); i++)                                            \
            out_ref[i] = FREF(xs[i]);                                            \
    t_ref = OBZ_now_s() - t0;                                                    \
                                                                                \
    /* run SIMD */                                                               \
    t0 = OBZ_now_s();                                                            \
    for (int r = 0; r < RUNS; r++)                                               \
        FSIMD(xs, out_simd, N);                                                  \
    t_simd = OBZ_now_s() - t0;                                                   \
                                                                                \
    OBZ_stats_reset(&st);                                                        \
    for (int i = 0; i < (N); i++)                                                \
        OBZ_stats_add(&st, out_ref[i], out_simd[i]);                             \
                                                                                \
    printf("time_ref   = %.6f s\n", t_ref);                                      \
    printf("time_simd  = %.6f s\n", t_simd);                                     \
    printf("speedup    = %.3fx\n", t_ref / t_simd);                              \
    printf("throughput = %.3f Mops/s\n\n", OBZ_mops((double)N*RUNS, t_simd));    \
                                                                                \
    OBZ_stats_print("Error summary:", &st);                                      \
} while(0)

/* ================================================================
   BENCHMARK: SIMD SCALAR-WRAPPER
   For functions like:
       float sin_scalar_sse(float)
   ================================================================ */
#define OBZ_BENCH_SIMD_SCALAR(FREF, FSIMD, xs, out, N, RUNS) \
    OBZ_BENCH_SCALAR(FREF, FSIMD, xs, out, N, RUNS)

#endif /* OBZ_TEST_UTILS_H */
