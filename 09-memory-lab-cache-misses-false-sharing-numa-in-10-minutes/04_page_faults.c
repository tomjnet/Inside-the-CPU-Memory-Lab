/* Memory Performance: Cache Misses, False Sharing and NUMA - slide 4: tlb misses and page faults (C version of 04_page_faults.cpp) */
/* Build: make 04_page_faults_c */
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

#if defined(__linux__)
#include <sys/resource.h>
#endif

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

#if defined(__linux__)
static long minor_faults(void) {
    struct rusage u;
    memset(&u, 0, sizeof u);
    getrusage(RUSAGE_SELF, &u);
    return u.ru_minflt;
}
#endif

#define kPage  ((size_t)4096)
#define kBytes ((size_t)64 << 20)   /* 16384 pages */

static int g_alloc_failed = 0;

/* fresh buffer: the first write to each page is a minor page fault.
   malloc without a fill is the C twin of make_unique_for_overwrite; C has no destructor, so free by hand */
static void fresh_pass(void *ctx) {
    (void)ctx;
    char *p = malloc(kBytes);
    if (p == NULL) { g_alloc_failed = 1; return; }
    for (size_t i = 0; i < kBytes; i += kPage) p[i] = 1;
    sink((unsigned char)p[kBytes - kPage]);
    free(p);
}

/* mapped buffer: no faults left, only TLB misses and cache misses */
static void mapped_pass(void *ctx) {
    char *buf = ctx;
    for (size_t i = 0; i < kBytes; i += kPage) buf[i] = 1;
    sink((unsigned char)buf[kBytes - kPage]);
}

int main(void) {
    /* the mapped buffer of the slide: the fill writes every page, so all of them are mapped before any timing */
    char *buf = malloc(kBytes);
    if (buf == NULL) {
        printf("FAIL: out of memory\n");
        return 1;
    }
    memset(buf, 0, kBytes);

    BenchResult fresh = bench_ns(fresh_pass, NULL, 3, 21);
    BenchResult mapped = bench_ns(mapped_pass, buf, 3, 21);
    if (g_alloc_failed) {
        printf("FAIL: out of memory\n");
        free(buf);
        return 1;
    }

    const double pages = (double)(kBytes / kPage);
    printf("one write per 4 KiB page, %zu pages, median of 21 runs (this machine):\n", kBytes / kPage);
    printf("  fresh buffer : %9.1f ms, %.1f ns per page (allocation, page faults, release)\n",
           fresh.median_ns / 1e6, fresh.median_ns / pages);
    printf("  mapped buffer: %9.1f ms, %.1f ns per page (TLB misses and cache misses only)\n",
           mapped.median_ns / 1e6, mapped.median_ns / pages);
    printf("  ratio        : %.1fx\n", fresh.median_ns / mapped.median_ns);

#if defined(__linux__)
    /* the kernel's own count: minor faults of one more fresh pass */
    const long before = minor_faults();
    fresh_pass(NULL);
    const long faults = minor_faults() - before;
    printf("minor page faults of one fresh pass (getrusage): %ld, one per page would be %zu\n",
           faults, kBytes / kPage);
    if (faults < (long)(kBytes / kPage) / 2)
        printf("  far fewer than one per page: transparent huge pages mapped 2 MiB at a time\n");
#else
    printf("this sample needs Linux: the minor page fault count of one fresh pass, read with getrusage\n");
#endif
    free(buf);
    return 0;
}
