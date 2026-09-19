/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 12: mmap under malloc (C version of 12_malloc_big.cpp) */
/* Build: make 12_malloc_big_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- timing: now_ns and sink from the harness ---- */
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
static void sink(uint64_t x) { g_sink = g_sink + x; }   /* keeps the pointer alive: the pair cannot be deleted */

static void show(const char *name, void *p) {
    printf("  %s  0x%" PRIxPTR "\n", name, (uintptr_t)p);
}

/* Average cost of one malloc and free pair of the given size, in nanoseconds. */
static double pair_ns(size_t bytes, int pairs) {
    const double t0 = now_ns();
    for (int i = 0; i < pairs; ++i) {
        void *p = malloc(bytes);
        sink((uint64_t)(uintptr_t)p);
        free(p);
    }
    const double t1 = now_ns();
    return (t1 - t0) / pairs;
}

static int global_anchor = 0;               /* something in the data region, to measure distances from */

int main(void) {
    /* small blocks come from the heap, big ones from their own mmap */
    void *small = malloc(64);               /* heap: no system call */
    void *big = malloc((size_t)256 << 20);  /* 256 MiB: one mmap inside */
    show("small", small);                   /* just above the data */
    show("big  ", big);                     /* far away, near the libs */
    if (small == NULL || big == NULL) {
        printf("malloc returned NULL: this machine refused 256 MiB of address space\n");
        free(big);
        free(small);
        return 0;
    }

    const uintptr_t data_at = (uintptr_t)&global_anchor;
    const uintptr_t small_at = (uintptr_t)small;
    const uintptr_t big_at = (uintptr_t)big;
    const double mib = 1024.0 * 1024.0;
    printf("  distance data to small  %.1f MiB\n",
           (double)(small_at > data_at ? small_at - data_at : data_at - small_at) / mib);
    printf("  distance data to big    %.1f MiB\n",
           (double)(big_at > data_at ? big_at - data_at : data_at - big_at) / mib);

    free(big);                              /* munmap: back to the kernel */
    free(small);                            /* back to the free list */

    const double small_ns = pair_ns(64, 200000);
    const double big_ns = pair_ns((size_t)256 << 20, 2000);
    printf("\nmalloc plus free, average per pair on this machine:\n");
    printf("  64 bytes   %.1f ns  (free list, no system call)\n", small_ns);
    printf("  256 MiB    %.1f ns  (a system call to map and one to unmap)\n", big_ns);
    printf("  ratio      %.1fx\n", big_ns / small_ns);
#if defined(__linux__)
    printf("glibc maps blocks from 128 KiB by default (M_MMAP_THRESHOLD), and the threshold adapts\n");
#else
    printf("this is not Linux: the C library here has its own threshold, the idea is the same\n");
#endif
    return 0;
}
