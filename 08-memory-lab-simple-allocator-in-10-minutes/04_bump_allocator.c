/* Build a Simple Memory Allocator: Understanding malloc and new - slide 4: bump allocator: the fastest one (C version of 04_bump_allocator.cpp) */
/* Build: make 04_bump_allocator_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdbool.h>
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

/* ---- slide 3: the arena and the interface ---- */
#define ARENA ((size_t)4096)                 /* one page, fixed forever */
#define ALIGN ((size_t)16)                   /* what malloc promises */
static _Alignas(16) unsigned char arena[ARENA];

/* round n up to a multiple of 16: 1 -> 16, 16 -> 16, 17 -> 32 */
static size_t align_up(size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

void *allocate(size_t size);                 /* NULL when it is full */
void  deallocate(void *ptr);                 /* no size: we store it */

/* ---- slide 4: bump allocator ---- */
static size_t top = 0;                       /* first free byte */
void *allocate(size_t size) {                /* O(1): add and compare */
    size_t need = align_up(size);
    if (need > ARENA - top) return NULL;     /* arena full */
    void *p = arena + top;
    top += need;                             /* the bump */
    return p;
}
static void reset(void) { top = 0; }         /* frees everything: O(1) */

static void *held[128];                      /* the malloc side keeps its blocks alive, as the bump side does */

/* C has no lambdas: each measured body is a function, the round count travels in ctx */
static void bump_rounds(void *ctx) {
    int rounds = *(const int *)ctx;
    for (int r = 0; r < rounds; ++r) {
        void *last = NULL;
        for (int i = 0; i < 128; ++i) last = allocate(24);
        sink((uint64_t)(uintptr_t)last);
        reset();
    }
}
static void libc_rounds(void *ctx) {
    int rounds = *(const int *)ctx;
    for (int r = 0; r < rounds; ++r) {
        for (int i = 0; i < 128; ++i) held[i] = malloc(24);
        sink((uint64_t)(uintptr_t)held[127]);
        for (int i = 0; i < 128; ++i) free(held[i]);
    }
}

int main(void) {
    bool ok = true;
    printf("three allocations, top after each one:\n");
    const size_t sizes[] = { 100, 200, 24 };
    for (size_t i = 0; i < 3; ++i) {
        void *p = allocate(sizes[i]);
        if (p == NULL) { ok = false; continue; }
        size_t off = (size_t)((unsigned char *)p - arena);
        printf("  allocate(%zu) -> arena + %zu, top = %zu\n", sizes[i], off, top);
        if (off % ALIGN != 0) ok = false;
    }
    if (top != 352) ok = false;              /* 112 + 208 + 32, the numbers of the figure */

    size_t count = 0;
    while (allocate(24) != NULL) ++count;    /* fill the rest: 32 bytes per block */
    printf("then %zu more blocks of 24 bytes fit, top = %zu, next allocate: NULL\n", count, top);
    reset();
    printf("reset(): top = %zu, the whole arena is free again in one step\n\n", top);

    /* 128 blocks of 24 bytes, release them all, 1000 times: bump + reset against malloc + free */
    int rounds = 1000;
    BenchResult bump = bench_ns(bump_rounds, &rounds, 3, 21);
    BenchResult libc = bench_ns(libc_rounds, &rounds, 3, 21);
    double per_bump = bump.median_ns / (rounds * 128.0);
    double per_libc = libc.median_ns / (rounds * 128.0);
    printf("this machine, median per block: bump %g ns, malloc + free %g ns", per_bump, per_libc);
    if (per_bump > 0.0) printf(", ratio %g", per_libc / per_bump);
    printf("\n(bump cannot free one block: that is what it pays for the speed)\n");
    return ok ? 0 : 1;
}
