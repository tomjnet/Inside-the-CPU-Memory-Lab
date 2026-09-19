/* Memory Alignment and Locality: Why Data Layout Matters - slide 8: timing the one field scan (C version of 08_aos_vs_soa.cpp) */
/* Build: make 08_aos_vs_soa_c */
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

/* fixed seed xorshift64 in place of std::mt19937: every run does the same work */
static uint64_t rng_state = 12345;
static uint64_t xorshift64(void) {
    uint64_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return rng_state = x;
}

typedef struct {                  /* AoS: 64 bytes, one cache line each */
    double x, y, z, vx, vy, vz, mass, charge;
} Particle;
typedef struct {                  /* SoA: one contiguous array per field */
    double *x, *y, *z, *vx, *vy, *vz, *mass, *charge;
} Particles;

/* C has no lambda: the context struct carries what the C++ lambda captured */
typedef struct { const Particle *a; const Particles *b; size_t n; } ScanCtx;

static void scan_aos(void *ctx) {      /* sum of x: 1 cache line per element */
    const ScanCtx *c = ctx;
    double s = 0;
    for (size_t i = 0; i < c->n; ++i) s += c->a[i].x;
    sink((uint64_t)s);
}
static void scan_soa(void *ctx) {      /* sum of x: 1 cache line per 8 */
    const ScanCtx *c = ctx;
    double s = 0;
    for (size_t i = 0; i < c->n; ++i) s += c->b->x[i];
    sink((uint64_t)s);
}

int main(void) {
    const size_t n = 1000000;                      /* 64 MB in each layout */
    int rc = 1;
    Particle *a = calloc(n, sizeof *a);
    Particles b = {0};
    double **fields[] = {&b.x, &b.y, &b.z, &b.vx, &b.vy, &b.vz, &b.mass, &b.charge};
    const size_t nfields = sizeof fields / sizeof fields[0];
    for (size_t f = 0; f < nfields; ++f) *fields[f] = calloc(n, sizeof(double));
    if (a == NULL) goto oom;
    for (size_t f = 0; f < nfields; ++f)
        if (*fields[f] == NULL) goto oom;

    for (size_t i = 0; i < n; ++i) {               /* whole numbers, so both sums are exact and must be equal */
        Particle p = {0};
        p.x = (double)(xorshift64() % 10);
        p.y = (double)(xorshift64() % 10);
        p.z = (double)(xorshift64() % 10);
        p.mass = 1.0;
        a[i] = p;
        b.x[i] = p.x;
        b.y[i] = p.y;
        b.z[i] = p.z;
        b.mass[i] = p.mass;
    }

    ScanCtx ctx = { a, &b, n };
    BenchResult aos = bench_ns(scan_aos, &ctx, 3, 21);
    BenchResult soa = bench_ns(scan_soa, &ctx, 3, 21);

    double sum_aos = 0, sum_soa = 0;
    for (size_t i = 0; i < n; ++i) sum_aos += a[i].x;
    for (size_t i = 0; i < n; ++i) sum_soa += b.x[i];
    const int equal = (uint64_t)sum_aos == (uint64_t)sum_soa;

    printf("sizeof(Particle) = %zu bytes, n = %zu\n", sizeof(Particle), n);
    printf("bytes that travel for the sum of x: AoS %zu MB, SoA %zu MB\n", n * sizeof(Particle) / 1000000,
           n * sizeof(double) / 1000000);
    printf("AoS: min %g ms, median %g ms (this machine)\n", aos.min_ns / 1e6, aos.median_ns / 1e6);
    printf("SoA: min %g ms, median %g ms (this machine)\n", soa.min_ns / 1e6, soa.median_ns / 1e6);
    printf("ratio AoS / SoA (median): %gx (this machine)\n", aos.median_ns / soa.median_ns);
    printf("sum of x: AoS %g, SoA %g%s", sum_aos, sum_soa, equal ? " (equal)\n" : " (DIFFERENT)\n");
    rc = equal ? 0 : 1;
    goto done;
oom:
    printf("FAIL: out of memory\n");
done:
    /* no destructor in C: every array is freed by hand on every path */
    free(a);
    for (size_t f = 0; f < nfields; ++f) free(*fields[f]);
    return rc;
}
