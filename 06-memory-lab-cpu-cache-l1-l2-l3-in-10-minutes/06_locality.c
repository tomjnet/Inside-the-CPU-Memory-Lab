/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 6: spatial and temporal locality (C version of 06_locality.cpp) */
/* Build: make 06_locality_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

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

static long long sum_stride(const int *a, size_t n, size_t step) {
    long long sum = 0;              /* temporal: sum and i are reused */
    for (size_t i = 0; i < n; i += step)
        sum += a[i];                /* spatial: a[i + 1] sits next door */
    return sum;
}

/* C has no lambda: the context struct carries what the C++ lambda captured */
typedef struct { const int *data; size_t n; size_t step; } StrideCtx;

static void run_stride(void *ctx) {
    const StrideCtx *c = ctx;
    sink((uint64_t)sum_stride(c->data, c->n, c->step));
}

int main(void) {
    const size_t n = (size_t)1 << 24;                /* 16M ints = 64 MiB: far bigger than any L2 */
    int *data = malloc(n * sizeof *data);
    if (data == NULL) {
        printf("FAIL: out of memory\n");
        return 1;
    }
    for (size_t i = 0; i < n; ++i) data[i] = 1;

    /* step 1:  16 ints per cache line, at worst 1 miss per 16 reads */
    /* step 16: every read lands on a new line, 64 bytes for 4 used */
    StrideCtx dctx = { data, n, 1 };
    StrideCtx sctx = { data, n, 16 };
    BenchResult dense  = bench_ns(run_stride, &dctx, 3, 21);
    BenchResult sparse = bench_ns(run_stride, &sctx, 3, 21);
    /* per int read, step 16 is often several times slower */

    const double dense_reads = (double)n;
    const double sparse_reads = (double)(n / 16);
    const double dense_per = dense.median_ns / dense_reads;
    const double sparse_per = sparse.median_ns / sparse_reads;

    printf("sum_stride over %zu ints (%zu MiB), this machine\n", n, n * sizeof(int) / (1024 * 1024));
    printf("  step 1 : %g reads, median %g ms, %g ns per int read\n", dense_reads, dense.median_ns / 1e6,
           dense_per);
    printf("  step 16: %g reads, median %g ms, %g ns per int read\n", sparse_reads, sparse.median_ns / 1e6,
           sparse_per);
    printf("  per int read, step 16 / step 1: %gx (this machine)\n", sparse_per / dense_per);
    printf("both walks touch the same %zu cache lines: step 1 uses 16 ints of each, step 16 one\n", n / 16);

    const int ok = sum_stride(data, n, 1) == (long long)n &&
                   sum_stride(data, n, 16) == (long long)(n / 16);
    free(data);
    if (!ok) {
        printf("FAIL: a sum is wrong\n");
        return 1;
    }
    printf("check: both sums are exact\n");
    return 0;
}
