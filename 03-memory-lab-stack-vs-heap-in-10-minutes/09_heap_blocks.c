/* Stack vs Heap: Where Does Your Data Actually Live? - slide 9: lab: heap blocks and their hidden headers (C version of 09_heap_blocks.cpp) */
/* Build: make 09_heap_blocks_c */
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
#if defined(__linux__)
#include <malloc.h>
#endif

/* signed distance in bytes from block a to block b (the blocks are unrelated arrays: compare as integers) */
static long long gap(const void *a, const void *b) {
    return (long long)(uintptr_t)b - (long long)(uintptr_t)a;
}

int main(void) {
    char *blocks[5];
    for (int i = 0; i < 5; ++i) {
        blocks[i] = malloc(24);               /* ask for 24 B, five times */
        if (blocks[i] == NULL) {
            for (int j = 0; j < i; ++j) free(blocks[j]);
            return 1;
        }
    }
    printf("%p  first block\n", (void *)blocks[0]);
    for (int i = 1; i < 5; ++i) {
        printf("%p  gap %lld B\n", (void *)blocks[i],
               gap(blocks[i - 1], blocks[i]));  /* often 32 */
    }                                         /* 24 B + header, 16 B aligned */
    int on_stack = 0;                         /* another region, far away */
    printf("%p  a local on the stack\n", (void *)&on_stack);
    printf("distance from the first heap block to the local: %lld MiB (on_stack = %d)\n",
           gap(blocks[0], &on_stack) / (1024 * 1024), on_stack);
    for (int i = 0; i < 5; ++i) free(blocks[i]);   /* every malloc needs one free */

    printf("\nWe asked for 24 B. A constant gap larger than 24 is the header plus the rounding\n");
    printf("to the allocator's alignment (glibc: 32). Other allocators place blocks differently.\n\n");

#if defined(__linux__)
    printf("what glibc really hands out (malloc_usable_size):\n");
    static const size_t requests[] = { 1, 24, 25, 100 };
    for (size_t k = 0; k < sizeof requests / sizeof requests[0]; ++k) {
        void *p = malloc(requests[k]);
        if (p == NULL) return 1;
        printf("  malloc(%zu) -> %zu usable bytes\n", requests[k], malloc_usable_size(p));
        free(p);
    }
#else
    printf("this sample needs Linux: malloc_usable_size would show that malloc(1) and malloc(24)\n");
    printf("both hand out 24 usable bytes inside a 32 byte block\n");
#endif
    return 0;
}
