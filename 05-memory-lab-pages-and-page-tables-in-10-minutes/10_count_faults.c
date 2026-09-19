/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 10: count minor faults with getrusage (C version of 10_count_faults.cpp) */
/* Build: make 10_count_faults_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdio.h>

#if defined(__linux__)
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>

static long minor_faults(void) {             /* ru_majflt: from disk */
    struct rusage u = {0}; getrusage(RUSAGE_SELF, &u); return u.ru_minflt;
}

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

int main(void) {
    const size_t expected = ((size_t)64 << 20) / 4096;
    printf("64 MiB / 4 KiB = %zu pages: expect about that many minor faults on the first touch\n", expected);

#if defined(__linux__)
    const size_t pages = 16384;                  /* 64 MiB of 4 KiB pages */
    char *p = mmap(NULL, pages * 4096,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        printf("mmap refused: %s\n", strerror(errno));
        return 0;
    }
    /* not on the slide: ask for 4 KiB pages, so a kernel with transparent huge pages set to "always"
       still gives one fault per page and the count is exact */
    if (madvise(p, pages * 4096, MADV_NOHUGEPAGE) != 0)
        printf("madvise(MADV_NOHUGEPAGE) refused: %s (the count may be lower)\n", strerror(errno));

    double t0 = now_ns();
    long before = minor_faults();
    for (size_t i = 0; i < pages; ++i) p[i * 4096] = 1;
    long first = minor_faults() - before;        /* about one per page */
    double first_ms = (now_ns() - t0) / 1e6;

    t0 = now_ns();
    before = minor_faults();
    for (size_t i = 0; i < pages; ++i) p[i * 4096] = 2;
    long second = minor_faults() - before;       /* about zero: mapped */
    double second_ms = (now_ns() - t0) / 1e6;

    printf("first touch : %ld minor faults, %g ms (this machine)\n", first, first_ms);
    printf("second touch: %ld minor faults, %g ms (this machine)\n", second, second_ms);
    if (second_ms > 0.0) printf("ratio       : %gx, the kernel filling page tables\n", first_ms / second_ms);
    printf("check bytes : %d\n", (int)p[0] + (int)p[(pages - 1) * 4096]);
    munmap(p, pages * 4096);
#else
    printf("this sample needs Linux: it would mmap 64 MiB, touch one byte per page and print about %zu"
           " minor faults from getrusage on the first pass and about 0 on the second\n", expected);
#endif
    return 0;
}
