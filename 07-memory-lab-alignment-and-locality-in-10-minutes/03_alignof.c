/* Memory Alignment and Locality: Why Data Layout Matters - slide 3: alignof in code (C version of 03_alignof.cpp) */
/* Build: make 03_alignof_c */
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

#if defined(_MSC_VER)
typedef double max_align_t;        /* MSVC C leaves it out of <stddef.h>; its C++ headers define it as double */
#endif

static double global_d = 2.71;     /* static storage, for the check at the end */

/* C has no templates: the caller passes the alignment the C++ version read from alignof(T) */
static uintptr_t remainder_of(const void *p, size_t align) {
    return (uintptr_t)p % align;
}

int main(void) {
    /* alignof(T): every address of a T is a multiple of it */
    printf("%zu\n", alignof(char));               /* 1 */
    printf("%zu\n", alignof(short));              /* 2 */
    printf("%zu\n", alignof(int));                /* 4 */
    printf("%zu\n", alignof(double));             /* 8 on x86 64 */
    printf("%zu\n", alignof(max_align_t));        /* 16 with gcc */

    double d = 3.14;                       /* check it: the low bits are 0 */
    uintptr_t addr = (uintptr_t)&d;
    printf("%zu\n", (size_t)(addr % alignof(double)));   /* always 0 */

    /* The same table with names, plus sizeof: for the fundamental types the two numbers match. */
    printf("\ntype            sizeof  alignof\n");
    printf("char            %zu       %zu\n", sizeof(char), alignof(char));
    printf("short           %zu       %zu\n", sizeof(short), alignof(short));
    printf("int             %zu       %zu\n", sizeof(int), alignof(int));
    printf("long long       %zu       %zu\n", sizeof(long long), alignof(long long));
    printf("double          %zu       %zu\n", sizeof(double), alignof(double));
    printf("void*           %zu       %zu\n", sizeof(void *), alignof(void *));
    printf("max_align_t     %zu      %zu   (MSVC reports 8 here; its 64 bit heap still aligns blocks to 16)\n",
           sizeof(max_align_t), alignof(max_align_t));

    /* The rule holds for every storage region: local, global and heap. */
    double *heap_d = malloc(sizeof *heap_d);
    if (heap_d == NULL) {
        printf("FAIL: out of memory\n");
        return 1;
    }
    *heap_d = 1.41;
    const size_t a = alignof(double);
    uintptr_t bad = remainder_of(&d, a) + remainder_of(&global_d, a) + remainder_of(heap_d, a);
    printf("\naddress %% alignof(double): local %zu, global %zu, heap %zu\n", (size_t)remainder_of(&d, a),
           (size_t)remainder_of(&global_d, a), (size_t)remainder_of(heap_d, a));
    printf("heap block %% 16: %zu   (malloc aligns every block for any fundamental type)\n",
           (size_t)((uintptr_t)heap_d % 16));
    printf("values: %g %g %g\n", d, global_d, *heap_d);
    free(heap_d);                      /* no unique_ptr in C: the free is ours */
    return bad == 0 ? 0 : 1;
}
