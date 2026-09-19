/* Memory Performance: Cache Misses, False Sharing and NUMA - slide 10: memory bandwidth with memcpy (C version of 10_memcpy_bandwidth.cpp) */
/* Build: make 10_memcpy_bandwidth_c */
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

/* fixed-seed splitmix64 stands in for std::mt19937(12345) */
static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

enum { kHops = 200000 };                        /* dependent reads per run */
#define kNodes ((size_t)1 << 24)                /* 16M indexes of 4 bytes: 64 MiB, bigger than any cache */
#define kBytes ((size_t)128 << 20)              /* 128 MiB */

/* every read tells where the next one is: the core cannot start hop n + 1 before hop n arrives */
static uint64_t follow(const uint32_t *next, int hops) {
    uint32_t i = 0;
    uint64_t sum = 0;
    for (int h = 0; h < hops; ++h) {
        i = next[i];
        sum += i;
    }
    return sum;
}

typedef struct { char *dst; const char *src; } CopyCtx;

static void copy_run(void *p) {                /* one read, one write stream */
    CopyCtx *c = p;
    memcpy(c->dst, c->src, kBytes);
    sink((unsigned char)c->dst[kBytes - 1]);
}

static void chase_run(void *p) { sink(follow(p, kHops)); }

int main(void) {
    int rc = 1;
    uint32_t *next = malloc(kNodes * sizeof *next);
    char *src = malloc(kBytes);
    char *dst = malloc(kBytes);
    if (next == NULL || src == NULL || dst == NULL) {
        printf("FAIL: out of memory\n");
        goto done;                              /* C has no destructor: one cleanup label frees all three */
    }

    /* one random cycle through all the nodes (the Sattolo shuffle, fixed seed): no short loops, nothing to prefetch */
    for (size_t i = 0; i < kNodes; ++i) next[i] = (uint32_t)i;
    uint64_t state = 12345;
    for (size_t i = kNodes - 1; i > 0; --i) {
        const size_t j = (size_t)(splitmix64(&state) % (uint64_t)i);
        uint32_t t = next[i];
        next[i] = next[j];
        next[j] = t;
    }

    memset(src, 1, kBytes);                     /* already mapped */
    memset(dst, 0, kBytes);
    CopyCtx cc = { dst, src };
    BenchResult copy = bench_ns(copy_run, &cc, 3, 21);
    double gib_s = ((double)kBytes / 1073741824.0) / (copy.median_ns * 1e-9);
    /* latency: a chase through the same RAM, one dependent read at a time */
    BenchResult chase = bench_ns(chase_run, next, 3, 21);
    double ns_per_hop = chase.median_ns / kHops;

    printf("bandwidth: memcpy of 128 MiB, median %.2f ms = %.2f GiB per second (this machine)\n",
           copy.median_ns / 1e6, gib_s);
    printf("latency  : %d dependent reads over 64 MiB, %.2f ns per hop (this machine)\n", kHops, ns_per_hop);
    const double chase_mib_s = (4.0 / 1048576.0) / (ns_per_hop * 1e-9);
    printf("the chase delivers %.2f MiB of indexes per second: the same RAM, %.2fx less data per second than the stream\n",
           chase_mib_s, gib_s * 1024.0 / chase_mib_s);

    if (dst[0] != 1 || dst[kBytes - 1] != 1) {
        printf("FAIL: the copy did not arrive\n");
        goto done;
    }
    rc = 0;
done:
    free(next);
    free(src);
    free(dst);
    return rc;
}
