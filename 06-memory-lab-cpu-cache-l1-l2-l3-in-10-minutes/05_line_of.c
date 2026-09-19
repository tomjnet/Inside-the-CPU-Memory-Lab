/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 5: hit and miss: which line is my byte on? (C version of 05_line_of.cpp) */
/* Build: make 05_line_of_c */
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

#define LINE_BYTES ((uintptr_t)64)          /* bytes per cache line */

static uintptr_t line_of(const void *p) {   /* address / 64 */
    return (uintptr_t)p / LINE_BYTES;
}

static size_t lines_touched(const int *a, size_t n) {
    size_t lines = 1;                       /* first touch: one miss */
    for (size_t i = 1; i < n; ++i)          /* new line: next miss */
        if (line_of(&a[i]) != line_of(&a[i - 1])) ++lines;
    return lines;                           /* 16 ints share a line */
}

int main(void) {
    static int a[32];
    printf("int a[32]: %zu bytes, %zu bytes per int, %zu ints per 64 byte line\n\n",
           sizeof a, sizeof(int), (size_t)(LINE_BYTES / sizeof(int)));

    /* the line number of a few elements: neighbours share it, then it changes by one */
    const uintptr_t first = line_of(&a[0]);
    static const size_t picks[] = {0, 3, 4, 15, 16, 31};
    for (size_t k = 0; k < sizeof picks / sizeof picks[0]; ++k) {
        size_t i = picks[k];
        printf("  a[%zu] at %p  line %llu  (first line + %llu)\n", i, (const void *)&a[i],
               (unsigned long long)line_of(&a[i]), (unsigned long long)(line_of(&a[i]) - first));
    }
    printf("  the array does not have to start at the start of a line: a[0] is at byte %llu of its line\n\n",
           (unsigned long long)((uintptr_t)&a[0] % LINE_BYTES));

    /* how many lines a sequential walk touches: the upper limit on its misses */
    int ok = 1;
    static const size_t sizes[] = {16, 1000, 1000000};
    for (size_t k = 0; k < sizeof sizes / sizeof sizes[0]; ++k) {
        size_t n = sizes[k];
        int *v = malloc(n * sizeof *v);     /* C has no std::vector: malloc plus a length, then free */
        if (v == NULL) {
            printf("FAIL: out of memory\n");
            return 1;
        }
        for (size_t i = 0; i < n; ++i) v[i] = 1;
        const size_t lines = lines_touched(v, n);
        const size_t low = (n * sizeof(int) + LINE_BYTES - 1) / LINE_BYTES;   /* array starts on a line boundary */
        printf("  %zu ints: %zu cache lines touched, %zu reads, at most 1 miss per %g reads\n", n, lines, n,
               (double)n / (double)lines);
        if (lines < low || lines > low + 1) ok = 0;                     /* one more when it starts mid line */
        free(v);
    }

    printf("\nthis is arithmetic, not a measurement: a program cannot ask the cache what it holds\n");
    if (!ok) {
        printf("FAIL: the line count does not match bytes / 64\n");
        return 1;
    }
    printf("check: every count is bytes / 64 rounded up, plus at most one\n");
    return 0;
}
