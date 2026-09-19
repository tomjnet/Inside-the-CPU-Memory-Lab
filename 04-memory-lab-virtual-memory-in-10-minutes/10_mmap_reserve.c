/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 10: reserve 1 GiB with mmap (C version of 10_mmap_reserve.cpp) */
/* Build: make 10_mmap_reserve_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__linux__)
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

/* the POSIX half of the timing harness: only the Linux branch times anything */
static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

/* reserve 1 GiB of address space: one system call, no RAM yet */
static const size_t kGiB = (size_t)1 << 30;

#if defined(__linux__)
static char *reserve_gib(void) {
    void *p = mmap(NULL, kGiB,                  /* anywhere, 1 GiB */
                   PROT_READ | PROT_WRITE,      /* readable, writable */
                   MAP_PRIVATE | MAP_ANONYMOUS, /* no file behind it */
                   -1, 0);
    if (p == MAP_FAILED) return NULL;           /* errno says why */
    return (char *)p;                           /* zero pages, lazily */
}

/* The same call for any size, timed: the cost is the system call, not the length. */
static void time_reserve(const char *label, size_t bytes) {
    const double t0 = now_ns();
    void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    const double t1 = now_ns();
    if (p == MAP_FAILED) {
        printf("  %s  refused: %s (the overcommit check of the kernel said no)\n", label, strerror(errno));
        return;
    }
    printf("  %s  %.0f ns on this machine\n", label, t1 - t0);
    munmap(p, bytes);
}
#endif

int main(void) {
    printf("1 GiB = %zu bytes = %zu pages of 4 KiB\n", kGiB, kGiB / 4096);

#if defined(__linux__)
    char *base = reserve_gib();
    if (base == NULL) {
        printf("mmap refused 1 GiB: %s\n", strerror(errno));
        printf("on a normal Linux it returns an address at once and assigns no RAM\n");
        return 0;
    }
    printf("mmap returned 0x%" PRIxPTR ": a new region in /proc/self/maps, no frame of RAM behind it yet\n",
           (uintptr_t)base);
    printf("first byte reads as %d: anonymous pages start as zeros\n", (int)base[0]);
    munmap(base, kGiB);                         /* give the range back */

    printf("\nthe cost does not follow the size:\n");
    time_reserve("  1 MiB", (size_t)1 << 20);
    time_reserve("  1 GiB", kGiB);
    time_reserve(" 16 GiB", (size_t)16 << 30);
    time_reserve("  4 TiB", (size_t)4 << 40);
    printf("a refusal above is overcommit at work: the kernel promises a lot, not everything\n");
#else
    printf("this sample needs Linux: mmap would reserve 1 GiB of address space in one system call,\n");
    printf("in about the same time as 1 MiB, because no RAM is assigned until a page is touched\n");
#endif
    return 0;
}
