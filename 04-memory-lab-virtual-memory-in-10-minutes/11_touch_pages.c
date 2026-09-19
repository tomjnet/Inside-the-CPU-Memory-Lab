/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 11: touch 16 pages, read the resident size (C version of 11_touch_pages.cpp) */
/* Build: make 11_touch_pages_c */
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

/* resident pages of this process: the second field of statm */
static long resident_pages(void) {
    FILE *statm = fopen("/proc/self/statm", "r");
    long size = 0, resident = 0;
    if (statm == NULL) return 0;
    if (fscanf(statm, "%ld %ld", &size, &resident) != 2) resident = 0;   /* both in 4 KiB pages */
    fclose(statm);
    return resident;
}
#endif

/* touch 16 pages out of 262144: one page fault each */
static void touch(char *base) {
    for (size_t i = 0; i < 16; ++i)
        base[i * 4096] = 1;                 /* first write maps a page */
}

#if defined(__linux__)
/* The first field of the same file: the whole address space, in pages. */
static long total_pages(void) {
    FILE *statm = fopen("/proc/self/statm", "r");
    long size = 0;
    if (statm == NULL) return 0;
    if (fscanf(statm, "%ld", &size) != 1) size = 0;
    fclose(statm);
    return size;
}

static void report(const char *when, long total0, long resident0) {
    long total = total_pages() - total0;
    long resident = resident_pages() - resident0;
    printf("  %s  total %ld pages, resident %ld pages (change since the start)\n", when, total, resident);
}
#endif

int main(void) {
#if defined(__linux__)
    const size_t kGiB = (size_t)1 << 30;
    printf("reading /proc/self/statm, in pages of 4 KiB\n");
    fflush(stdout);
    (void)resident_pages();                 /* warm up: the first fopen touches pages of its own */
    const long total0 = total_pages();
    const long resident0 = resident_pages();
    printf("start: total %ld pages, resident %ld pages of 4 KiB\n", total0, resident0);
    if (total0 == 0) {
        printf("/proc/self/statm is not readable here: on a normal Linux the lines below show the change\n");
    }

    void *p = mmap(NULL, kGiB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        printf("mmap refused 1 GiB: %s\n", strerror(errno));
        printf("on a normal Linux: total grows by 262144 pages, resident by about 16 after the touch\n");
        return 0;
    }
    char *base = (char *)p;
    report("after mmap 1 GiB  ", total0, resident0);

    touch(base);
    report("after 16 touches  ", total0, resident0);

    long sum = 0;
    for (size_t i = 0; i < 16; ++i) sum += base[i * 4096];
    printf("  the 16 bytes read back as %ld (every other byte of the GiB is still a promise)\n", sum);

    munmap(base, kGiB);
    report("after munmap      ", total0, resident0);
    printf("about 16 pages = 64 KiB of RAM for 1 GiB of address space; a page or two of noise is normal\n");
#else
    static char buffer[16 * 4096];
    touch(buffer);
    printf("this sample needs Linux: it would mmap 1 GiB, touch 16 pages and read /proc/self/statm\n");
    printf("expected there: total +262144 pages after the mmap, resident about +16 after the touch\n");
    printf("here the 16 writes went to a static buffer (first byte %d): there is no /proc/self/statm to ask how much of it is resident\n",
           (int)buffer[0]);
#endif
    return 0;
}
