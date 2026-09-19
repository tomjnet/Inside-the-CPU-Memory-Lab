/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 11: huge pages of 2 mib (C version of 11_huge_pages.cpp) */
/* Build: make 11_huge_pages_c */
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

static long minor_faults(void) {
    struct rusage u = {0}; getrusage(RUSAGE_SELF, &u); return u.ru_minflt;
}

/* the baseline of slide 10: the same touch loop on 4 KiB pages */
static long small_page_faults(size_t bytes) {
    char *q = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (q == MAP_FAILED) return -1;
    if (madvise(q, bytes, MADV_NOHUGEPAGE) != 0) printf("(MADV_NOHUGEPAGE refused, baseline may be low)\n");
    long before = minor_faults();
    for (size_t i = 0; i < bytes; i += 4096) q[i] = 1;
    long n = minor_faults() - before;
    munmap(q, bytes);
    return n;
}
#endif

int main(void) {
    /* the arithmetic of the figure, on every platform */
    const size_t tlb_entries = 64;
    printf("TLB reach with %zu entries: %zu KiB with 4 KiB pages, %zu MiB with 2 MiB pages\n",
           tlb_entries, tlb_entries * 4, tlb_entries * 2);
    printf("2 MiB / 4 KiB = %zu: one huge page replaces one whole page table\n\n", ((size_t)2 << 20) / 4096);

#if defined(__linux__)
    FILE *thp = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
    char mode[256];
    if (thp != NULL && fgets(mode, sizeof mode, thp) != NULL) {
        mode[strcspn(mode, "\n")] = '\0';        /* fgets keeps the newline, getline did not */
        printf("transparent huge pages: %s\n", mode);
    } else {
        printf("transparent huge pages: no /sys file here, the hint will probably be refused\n");
    }
    if (thp != NULL) fclose(thp);

    /* the same 64 MiB, but ask for transparent huge pages of 2 MiB */
    const size_t bytes = (size_t)64 << 20;
    char *p = mmap(NULL, bytes,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        printf("mmap refused: %s\n", strerror(errno));
        return 0;
    }
    if (madvise(p, bytes, MADV_HUGEPAGE) != 0)   /* a hint, may be refused */
        printf("madvise(MADV_HUGEPAGE) refused: %s"
               " (on a kernel with huge pages you would see far fewer faults below)\n", strerror(errno));
    long before = minor_faults();
    for (size_t i = 0; i < bytes; i += 4096) p[i] = 1;
    long huge = minor_faults() - before;         /* up to 512 times fewer */
    /* one TLB entry now covers 2 MiB instead of 4 KiB */
    munmap(p, bytes);

    long small = small_page_faults(bytes);
    printf("minor faults touching 64 MiB, 4 KiB pages      : %ld\n", small);
    printf("minor faults touching 64 MiB, huge pages hinted: %ld\n", huge);
    if (small > 0 && huge > 0)
        printf("ratio: %gx fewer faults on this machine (512x is the limit; "
               "the unaligned ends of the mapping stay on 4 KiB pages)\n", (double)small / (double)huge);
#else
    printf("this sample needs Linux: it would touch 64 MiB after madvise(MADV_HUGEPAGE) and print far fewer "
           "minor faults than the 16384 of the 4 KiB run\n");
#endif
    return 0;
}
