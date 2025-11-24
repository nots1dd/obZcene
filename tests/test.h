#ifndef MHM_TEST_UTILS_H
#define MHM_TEST_UTILS_H

#include <math.h>
#include <time.h>

/* ----------------- Timer utility ----------------- */
static inline double mhm_now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ----------------- Stats ----------------- */
typedef struct {
    double sum_abs;
    double sum_sq;
    double max_abs;
    long long n;
} mhm_stats_t;

static inline void mhm_stats_reset(mhm_stats_t *s) {
    s->sum_abs = s->sum_sq = s->max_abs = 0;
    s->n = 0;
}

static inline void mhm_stats_add(mhm_stats_t *s, double diff) {
    double a = fabs(diff);
    s->sum_abs += a;
    s->sum_sq += a * a;
    if (a > s->max_abs) s->max_abs = a;
    s->n++;
}

/* ---------------- Benchmark Macro with speedup ----------------
   TYPE: float/double
   FUNC_REF: scalar reference function
   FUNC_SIMD: SIMD/vectorized function
   ARGS: arguments to FUNC_SIMD (comma-separated)
   NAME: string name to print
   RUNS: number of repetitions
   xs/out/N: data arrays
   t_ref: time taken by scalar reference function
------------------------------------------------------------------ */
#define BENCH_SIMD_TYPE(TYPE, FUNC_REF, FUNC_SIMD, ARGS, NAME, RUNS, xs, out, N, t_ref) do { \
    mhm_stats_t s;                                                                          \
    mhm_stats_reset(&s);                                                                    \
    double t0 = mhm_now_s();                                                                \
    for (int r = 0; r < RUNS; r++)                                                         \
        FUNC_SIMD ARGS;                                                                     \
    double t_simd = mhm_now_s() - t0;                                                      \
                                                                                             \
    /* compute errors */                                                                    \
    for (int i = 0; i < N; i++)                                                            \
        mhm_stats_add(&s, (double)(FUNC_REF(xs[i]) - out[i]));                             \
                                                                                             \
    printf("%-25s time=%.6f speedup=%.3fx\n", NAME, t_simd, t_ref / t_simd);              \
    printf("  error: max=%.6e mean=%.6e rms=%.6e\n\n",                                      \
           s.max_abs, s.sum_abs / N, sqrt(s.sum_sq / N));                                   \
} while(0)

/* ----------------- Benchmark Macro ----------------- */
/* Compares func1 and func2 over N elements.
   func1: reference function (e.g., std sqrtf)
   func2: tested function (e.g., SIMD)
   xs: input array
   out: output array for func2
   N: number of elements
   RUNS: how many repeated runs for timing
*/
#define MHM_BENCH_COMPARE(func1, func2, xs, out, N, RUNS) do { \
    double t0, t1_ref, t2_test; \
    mhm_stats_t s; \
\
    /* reference run */ \
    t0 = mhm_now_s(); \
    for (int r=0;r<RUNS;r++) \
        for (int i=0;i<(N);i++) \
            out[i] = func1(xs[i]); \
    t1_ref = mhm_now_s() - t0; \
\
    /* tested function run */ \
    t0 = mhm_now_s(); \
    for (int r=0;r<RUNS;r++) \
        for (int i=0;i<(N);i++) \
            out[i] = func2(xs[i]); \
    t2_test = mhm_now_s() - t0; \
\
    /* compute errors */ \
    mhm_stats_reset(&s); \
    for (int i=0;i<(N);i++) \
        mhm_stats_add(&s, (double)func1(xs[i]) - (double)out[i]); \
\
    printf("%s vs %s: time_ref=%.6f time_test=%.6f speedup=%.3fx\n", \
           #func1, #func2, t1_ref, t2_test, t1_ref/t2_test); \
    printf("  error: max=%.6e mean=%.6e rms=%.6e\n\n", \
           s.max_abs, s.sum_abs/(double)N, sqrt(s.sum_sq/(double)N)); \
} while(0)

#endif
