/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 11: find the edges of L1, L2 and L3 (C version of 11_size_sweep.cpp) */
/* Build: make 11_size_sweep_c */
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

/* fixed-seed splitmix64 stands in for std::mt19937(42) */
static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* 0 .. n-1 in a random order, fixed seed: every run and every toolchain does the same kind of work.
   The caller frees the result (C has no std::vector to do it). */
static uint32_t *shuffled_indexes(size_t n) {
    uint32_t *idx = malloc(n * sizeof *idx);
    if (idx == NULL) return NULL;
    for (size_t i = 0; i < n; ++i) idx[i] = (uint32_t)i;
    uint64_t state = 42;
    for (size_t i = n - 1; i > 0; --i) {       /* Fisher Yates */
        size_t j = (size_t)(splitmix64(&state) % (uint64_t)(i + 1));
        uint32_t t = idx[i];
        idx[i] = idx[j];
        idx[j] = t;
    }
    return idx;
}

typedef struct { const int *data; const uint32_t *idx; size_t n; } SumCtx;

static long long sum_by(const SumCtx *c) {
    long long sum = 0;
    for (size_t k = 0; k < c->n; ++k) sum += c->data[c->idx[k]];
    return sum;
}

static void run_sum(void *ctx) { sink((uint64_t)sum_by(ctx)); }

int main(void) {
    int ok = 1;
    printf("random walk, time per int by array size (this machine):\n");

    /* same random walk, four array sizes: where does it stop fitting? */
    static const size_t sizes_kib[] = {16, 256, 4096, 65536};
    for (size_t k = 0; k < sizeof sizes_kib / sizeof sizes_kib[0]; ++k) {
        size_t kib = sizes_kib[k];
        size_t n = kib * 1024 / sizeof(int);
        int *data = malloc(n * sizeof *data);
        uint32_t *idx = shuffled_indexes(n);
        if (data == NULL || idx == NULL) {
            printf("FAIL: out of memory\n");
            free(data);
            free(idx);
            return 1;
        }
        for (size_t i = 0; i < n; ++i) data[i] = 1;
        SumCtx ctx = { data, idx, n };
        BenchResult t = bench_ns(run_sum, &ctx, 1, 5);
        printf("%zu KiB: %g ns per int\n", kib, t.median_ns / (double)n);
        if (sum_by(&ctx) != (long long)n) ok = 0;
        free(data);
        free(idx);
    }
    /* typical homes: 16 KiB in L1, 256 KiB in L2, 4 MiB in L3, then RAM */

    printf("\nhow to read it: the cost is flat while the array fits a level and steps up at each edge.\n");
    printf("compare the steps with your sizes: 08_cache_sizes or lscpu --caches (typical: 48K, 2048K, 8M to 64M)\n");
    printf("the index array uses cache too, so the edges are soft; in a VM the L3 is shared and the numbers jump\n");

    if (!ok) {
        printf("FAIL: a sum is wrong\n");
        return 1;
    }
    printf("check: every sum is exact\n");
    return 0;
}
