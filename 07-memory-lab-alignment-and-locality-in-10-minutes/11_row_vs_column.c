/* Memory Alignment and Locality: Why Data Layout Matters - slide 11: timing row versus column (C version of 11_row_vs_column.cpp) */
/* Build: make 11_row_vs_column_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- timing harness: the C twin of bench_ns and sink ---- */
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double now_ns(void) {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1e9 / (double)f.QuadPart;
}
#else
#include <time.h>
static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

static volatile uint64_t g_sink = 0;
static void sink(uint64_t x) { g_sink = g_sink + x; }   /* keeps the result alive */

typedef struct { double min_ns; double median_ns; } BenchResult;

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* warm up, repeat, keep the minimum and the median; at most 64 repeats */
static BenchResult bench_ns(void (*f)(void *), void *ctx, int warmup, int repeats) {
    double samples[64];
    if (repeats > 64) repeats = 64;
    for (int i = 0; i < warmup; ++i) f(ctx);
    for (int i = 0; i < repeats; ++i) {
        double t0 = now_ns();
        f(ctx);
        samples[i] = now_ns() - t0;
    }
    qsort(samples, (size_t)repeats, sizeof samples[0], cmp_double);
    BenchResult r = { samples[0], samples[repeats / 2] };
    return r;
}

#define N ((size_t)2048)                 /* N x N ints: 16 MiB */

/* C has no lambda: the matrix pointer is the context */
static void walk_by_row(void *ctx) {    /* stride 4 bytes: sequential */
    const int *m = ctx;
    uint64_t s = 0;
    for (size_t r = 0; r < N; ++r)
        for (size_t c = 0; c < N; ++c) s += (uint64_t)m[r * N + c];
    sink(s);
}
static void walk_by_col(void *ctx) {    /* stride 8 KiB: a miss each */
    const int *m = ctx;
    uint64_t s = 0;
    for (size_t c = 0; c < N; ++c)
        for (size_t r = 0; r < N; ++r) s += (uint64_t)m[r * N + c];
    sink(s);
}

int main(void) {
    int *m = malloc(N * N * sizeof *m);  /* row major: m[r * N + c] */
    if (m == NULL) {
        printf("FAIL: out of memory\n");
        return 1;
    }
    for (size_t i = 0; i < N * N; ++i) m[i] = 1;
    /* the slide calls bench_ns(f); here 2 warm up runs and 11 repeats so the slow walk stays short */
    BenchResult by_row = bench_ns(walk_by_row, m, 2, 11);
    BenchResult by_col = bench_ns(walk_by_col, m, 2, 11);

    /* Both walks must add every cell exactly once: the sums are equal, only the order differs. */
    uint64_t sum_row = 0, sum_col = 0;
    for (size_t r = 0; r < N; ++r)
        for (size_t c = 0; c < N; ++c) sum_row += (uint64_t)m[r * N + c];
    for (size_t c = 0; c < N; ++c)
        for (size_t r = 0; r < N; ++r) sum_col += (uint64_t)m[r * N + c];

    printf("matrix %zu x %zu ints = %zu MiB, row major\n", N, N, N * N * sizeof(int) / (1024 * 1024));
    printf("stride of the inner loop: by row %zu bytes, by column %zu bytes\n", sizeof(int), N * sizeof(int));
    printf("by row:    min %g ms, median %g ms (this machine)\n", by_row.min_ns / 1e6, by_row.median_ns / 1e6);
    printf("by column: min %g ms, median %g ms (this machine)\n", by_col.min_ns / 1e6, by_col.median_ns / 1e6);
    printf("ratio column / row (median): %gx (this machine)\n", by_col.median_ns / by_row.median_ns);
    printf("sums: %" PRIu64 " and %" PRIu64 "%s", sum_row, sum_col,
           sum_row == sum_col ? " (equal)\n" : " (DIFFERENT)\n");
    free(m);                             /* no destructor in C: the free is ours */
    return sum_row == sum_col ? 0 : 1;
}
