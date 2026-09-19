/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 9: sequential sum: the baseline (C version of 09_sequential_sum.cpp) */
/* Build: make 09_sequential_sum_c */
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

/* C has no lambda: the data the C++ lambda captured travels in a context struct */
typedef struct { const int *data; const uint32_t *idx; size_t n; } SumCtx;

static long long sum_by(const SumCtx *c) {
    long long sum = 0;
    for (size_t k = 0; k < c->n; ++k) sum += c->data[c->idx[k]];   /* same code for */
    return sum;                                                     /* both orders */
}

static void run_sum(void *ctx) { sink((uint64_t)sum_by(ctx)); }

int main(void) {
    const size_t n = (size_t)1 << 22;          /* 4M ints = 16 MiB */
    int *data = malloc(n * sizeof *data);      /* bigger than L1 and L2 */
    uint32_t *order = malloc(n * sizeof *order);   /* order[i] = i for now */
    if (data == NULL || order == NULL) {
        printf("FAIL: out of memory\n");
        free(data);
        free(order);
        return 1;
    }
    for (size_t i = 0; i < n; ++i) {
        data[i] = 1;
        order[i] = (uint32_t)i;
    }

    SumCtx ctx = { data, order, n };
    BenchResult seq = bench_ns(run_sum, &ctx, 3, 21);
    /* 0, 1, 2, 3: one miss per 16 ints, the prefetcher hides even that */

    const double per_int = seq.median_ns / (double)n;
    printf("sequential sum through an index array, this machine\n");
    printf("  data : %zu ints, %zu MiB\n", n, n * sizeof(int) / (1024 * 1024));
    printf("  order: %u, %u, %u, %u to %u\n", (unsigned)order[0], (unsigned)order[1], (unsigned)order[2],
           (unsigned)order[3], (unsigned)order[n - 1]);
    printf("  min %g ms, median %g ms, %g ns per int\n", seq.min_ns / 1e6, seq.median_ns / 1e6, per_int);
    printf("  cache lines of data touched: %zu, one new line per 16 reads\n", n * sizeof(int) / 64);
    printf("this is the baseline: 10_shuffled_sum runs the same function with the same indexes, shuffled\n");

    const int ok = sum_by(&ctx) == (long long)n;
    free(data);
    free(order);
    if (!ok) {
        printf("FAIL: the sum is wrong\n");
        return 1;
    }
    printf("check: the sum is exact\n");
    return 0;
}
