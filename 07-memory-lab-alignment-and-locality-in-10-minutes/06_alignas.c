/* Memory Alignment and Locality: Why Data Layout Matters - slide 6: alignas(64): one object per cache line (C version of 06_alignas.cpp) */
/* Build: make 06_alignas_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <malloc.h>                /* _aligned_malloc, _aligned_free */
#endif
#if defined(_MSC_VER)
#pragma warning(disable : 4324)   /* "padded due to alignment specifier": that padding is the point here */
#endif

typedef struct {                  /* 8 bytes: eight fit in one line */
    uint64_t hits;
} Counter;
typedef struct {                  /* 64 bytes: one per cache line */
    _Alignas(64) uint64_t hits;   /* 8 bytes used, 56 of padding */
} LineCounter;

/* aligned heap memory: aligned_alloc wants a size that is a multiple of the alignment; Windows lacks it */
static void *alloc_aligned(size_t align, size_t size) {
#if defined(_WIN32)
    return _aligned_malloc(size, align);
#else
    return aligned_alloc(align, size);
#endif
}
static void free_aligned(void *p) {
#if defined(_WIN32)
    _aligned_free(p);
#else
    free(p);
#endif
}

int main(void) {
    static Counter     packed[4];     /* 32 bytes: one cache line */
    static LineCounter spread[4];     /* 256 bytes: four cache lines */

    printf("%zu %zu\n", sizeof(Counter), alignof(Counter));
    printf("%zu %zu\n", sizeof(LineCounter), alignof(LineCounter));   /* 64 64 */

    /* Which cache line does each element start in? (address / 64, relative to element 0) */
    printf("\nelement   packed: line, offset in line   spread: line, offset in line\n");
    int ok = 1;
    const uintptr_t p0 = (uintptr_t)&packed[0], s0 = (uintptr_t)&spread[0];
    for (int i = 0; i < 4; ++i) {
        packed[i].hits += 1;
        spread[i].hits += 1;
        uintptr_t p = (uintptr_t)&packed[i], s = (uintptr_t)&spread[i];
        printf("   %d              %zu, %zu                       %zu, %zu\n", i, (size_t)(p / 64 - p0 / 64),
               (size_t)(p % 64), (size_t)(s / 64 - s0 / 64), (size_t)(s % 64));
        if (s % 64 != 0) ok = 0;                   /* every LineCounter must start on a line boundary */
    }
    printf("packed[4] takes %zu bytes, spread[4] takes %zu bytes\n", sizeof(packed), sizeof(spread));

    /* malloc only promises max_align_t: an over aligned heap object needs the aligned allocator */
    LineCounter *heap = alloc_aligned(64, sizeof *heap);
    if (heap == NULL) {
        printf("FAIL: out of memory\n");
        return 1;
    }
    memset(heap, 0, sizeof *heap);
    heap->hits = packed[0].hits + spread[0].hits;
    printf("heap LineCounter: address %% 64 = %zu, hits %llu\n", (size_t)((uintptr_t)heap % 64),
           (unsigned long long)heap->hits);
    if ((uintptr_t)heap % 64 != 0) ok = 0;
    free_aligned(heap);                            /* no unique_ptr in C: the matching free is ours */

    printf("%s", ok ? "every LineCounter starts on a cache line boundary\n" : "ALIGNMENT BROKEN\n");
    return ok ? 0 : 1;
}
