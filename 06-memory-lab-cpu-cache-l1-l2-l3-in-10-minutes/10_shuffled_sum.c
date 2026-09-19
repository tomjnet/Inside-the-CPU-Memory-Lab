/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 10: shuffled index: same O(n), different time (C version of 10_shuffled_sum.cpp) */
/* Build: make 10_shuffled_sum_c */
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
#include <string.h>

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

/* fixed-seed splitmix64 stands in for std::mt19937(42): the shuffled indexes printed differ from the C++ run */
static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* Fisher Yates, the C twin of std::shuffle */
static void shuffle_u32(uint32_t *a, size_t n, uint64_t seed) {
    uint64_t state = seed;
    for (size_t i = n - 1; i > 0; --i) {
        size_t j = (size_t)(splitmix64(&state) % (uint64_t)(i + 1));
        uint32_t t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

/* C has no lambda: the data the C++ lambda captured travels in a context struct */
typedef struct { const int *data; const uint32_t *idx; size_t n; } SumCtx;

static long long sum_by(const SumCtx *c) {
    long long sum = 0;
    for (size_t k = 0; k < c->n; ++k) sum += c->data[c->idx[k]];
    return sum;
}

static void run_sum(void *ctx) { sink((uint64_t)sum_by(ctx)); }

/* how often the next read stays on the cache line of the previous one */
static double same_line(const uint32_t *idx, size_t n) {
    size_t same = 0;
    for (size_t i = 1; i < n; ++i)
        if (idx[i] / 16 == idx[i - 1] / 16) ++same;
    return 100.0 * (double)same / (double)(n - 1);
}

int main(void) {
    /* the baseline of slide 9, unchanged */
    const size_t n = (size_t)1 << 22;          /* 4M ints = 16 MiB */
    int *data = malloc(n * sizeof *data);
    uint32_t *order = malloc(n * sizeof *order);
    uint32_t *shuffled = malloc(n * sizeof *shuffled);
    if (data == NULL || order == NULL || shuffled == NULL) {
        printf("FAIL: out of memory\n");
        free(data);
        free(order);
        free(shuffled);
        return 1;
    }
    for (size_t i = 0; i < n; ++i) data[i] = (int)(i % 7);   /* not all equal: the sum is a real check */
    for (size_t i = 0; i < n; ++i) order[i] = (uint32_t)i;
    SumCtx sctx = { data, order, n };
    BenchResult seq = bench_ns(run_sum, &sctx, 3, 21);

    memcpy(shuffled, order, n * sizeof *order);   /* same n indexes */
    shuffle_u32(shuffled, n, 42);                 /* fixed seed */

    SumCtx rctx = { data, shuffled, n };
    BenchResult rnd = bench_ns(run_sum, &rctx, 3, 21);
    /* same n reads, same sum, same O(n): only the order changed */
    printf("random / sequential: %gx\n", rnd.median_ns / seq.median_ns);
    /* typical: several times slower, more as the array grows */

    const double dn = (double)n;
    printf("(this machine, %zu ints, %zu MiB)\n", n, n * sizeof(int) / (1024 * 1024));
    printf("  order   : %u, %u, %u, %u  median %g ms, %g ns per int\n", (unsigned)order[0], (unsigned)order[1],
           (unsigned)order[2], (unsigned)order[3], seq.median_ns / 1e6, seq.median_ns / dn);
    printf("  shuffled: %u, %u, %u, %u  median %g ms, %g ns per int\n", (unsigned)shuffled[0],
           (unsigned)shuffled[1], (unsigned)shuffled[2], (unsigned)shuffled[3], rnd.median_ns / 1e6,
           rnd.median_ns / dn);

    printf("  next read on the same 64 byte line: order %g %%, shuffled %g %%\n", same_line(order, n),
           same_line(shuffled, n));

    const long long a = sum_by(&sctx);
    const long long b = sum_by(&rctx);
    free(data);
    free(order);
    free(shuffled);
    if (a != b) {
        printf("FAIL: the two orders give different sums\n");
        return 1;
    }
    printf("check: same sum in both orders (%lld), same number of reads, only the order changed\n", a);
    return 0;
}
