/* Memory Performance: Cache Misses, False Sharing and NUMA - slide 3: cache misses: the working set (C version of 03_working_set.cpp) */
/* Build: make 03_working_set_c */
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

/* fixed-seed splitmix64 stands in for std::mt19937(12345) */
static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static double g_first_ns = 0.0;   /* the smallest working set is the reference for the ratio column */

static void report(size_t kib, double ns_per_read) {
    if (g_first_ns == 0.0) g_first_ns = ns_per_read;
    printf("  %7zu KiB working set: %7.2f ns per read, %.1fx the first row (this machine)\n",
           kib, ns_per_read, ns_per_read / g_first_ns);
}

/* the lambda's captures, spelled out: C passes them through a context pointer */
typedef struct { const uint32_t *idx; size_t n_idx; const uint32_t *data; size_t mask; } ReadCtx;

static void random_reads(void *p) {
    const ReadCtx *c = p;
    uint64_t sum = 0;
    for (size_t k = 0; k < c->n_idx; ++k) sum += c->data[c->idx[k] & c->mask];
    sink(sum);
}

int main(void) {
    /* the same one million random indexes for every size: fixed seed, so every run does the same work */
    const size_t n_idx = 1000000;
    uint32_t *idx = malloc(n_idx * sizeof *idx);
    if (idx == NULL) {
        printf("FAIL: out of memory\n");
        return 1;
    }
    uint64_t state = 12345;
    for (size_t k = 0; k < n_idx; ++k) idx[k] = (uint32_t)splitmix64(&state);

    printf("one million random reads, median of 21 runs:\n");

    /* one million random reads each time: only the working set grows */
    static const size_t sizes_kib[] = {16u, 512u, 8192u, 131072u};
    for (size_t s = 0; s < sizeof sizes_kib / sizeof sizes_kib[0]; ++s) {
        size_t kib = sizes_kib[s];
        size_t n = kib * 1024 / 4;
        uint32_t *data = malloc(n * sizeof *data);
        if (data == NULL) {
            printf("FAIL: out of memory\n");
            free(idx);
            return 1;
        }
        for (size_t i = 0; i < n; ++i) data[i] = 1;
        ReadCtx ctx = { idx, n_idx, data, n - 1 };     /* n is a power of two */
        BenchResult r = bench_ns(random_reads, &ctx, 3, 21);
        report(kib, r.median_ns / (double)n_idx);      /* ns per read */
        free(data);
    }
    /* 16 KiB lives in L1; 128 MiB misses every cache level */

    printf("the reads are independent, so the core overlaps several misses: the last row stays\n"
           "below the 100 ns latency of RAM. 10_memcpy_bandwidth chases pointers and pays it in full.\n");
    free(idx);
    return 0;
}
