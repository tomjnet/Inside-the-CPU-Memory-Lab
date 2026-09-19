/* Stack vs Heap: Where Does Your Data Actually Live? - slide 12: lab: a million new and delete pairs (C version of 12_time_new_delete.cpp) */
/* Build: make 12_time_new_delete_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

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

typedef struct { int x; int y; } Point;

enum { N = 1000000 };

/* The pointer is stored through a volatile: it escapes, so the compiler cannot remove the malloc and free pair. */
static Point *volatile g_keep = NULL;
static void keep(Point *p) { g_keep = p; }

static void heap_loop(void *ctx) {           /* N = 1000000 */
    (void)ctx;
    for (int i = 0; i < N; ++i) {
        Point *p = malloc(sizeof *p);         /* allocator call, header */
        if (p == NULL) continue;
        p->x = i;
        p->y = i;
        keep(p);                              /* escapes: pair not elided */
        sink((uint64_t)(p->x + p->y));
        free(p);                              /* allocator call again */
    }
}

static void stack_loop(void *ctx) {
    (void)ctx;
    for (int i = 0; i < N; ++i) { Point p = { i, i }; sink((uint64_t)(p.x + p.y)); }
}                                             /* same frame slot: no call */

int main(void) {
    BenchResult heap = bench_ns(heap_loop, NULL, 3, 21);
    BenchResult stack = bench_ns(stack_loop, NULL, 3, 21);

    printf("one million objects of %zu B, minimum and median of 21 runs, this machine\n", sizeof(Point));
    printf("  heap  (malloc + free): min %g ns per object, median %g ns\n",
           heap.min_ns / N, heap.median_ns / N);
    printf("  stack (a local)      : min %g ns per object, median %g ns\n",
           stack.min_ns / N, stack.median_ns / N);
    if (stack.median_ns > 0.0) {
        printf("  ratio heap / stack, this machine: %gx\n", heap.median_ns / stack.median_ns);
    }
    printf("\nThe stack loop still pays for the volatile sink; the heap loop pays for it too, plus two\n");
    printf("allocator calls. This is the best case for the heap: one thread, one size, freed at once.\n");
    printf("Take timings on real Linux hardware; a virtual machine shares its cores.\n");
    return 0;
}
