/* Memory Addresses and Pointers: What Is Really Stored? - slide 7: pointer arithmetic: walk an array (C version of 07_pointer_arithmetic.cpp) */
/* Build: make 07_pointer_arithmetic_c */
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

int main(void) {
    int a[4] = {10, 20, 30, 40};
    for (int *p = a; p != a + 4; ++p) {            /* ++p adds sizeof(int) */
        printf("%p: %d\n", (void *)p, *p);         /* addresses 4 apart */
    }
    int *first = a;
    int *last = a + 3;                             /* 12 bytes past a */
    printf("%td\n", last - first);                 /* 3 elements, not 12 bytes */
    /* one 64 byte cache line holds 16 of these ints */

    /* the same distance as plain numbers: the byte difference is the element difference times sizeof(int) */
    const uintptr_t lo = (uintptr_t)first;
    const uintptr_t hi = (uintptr_t)last;
    printf("\nbytes between first and last: %" PRIuPTR " = 3 * sizeof(int)\n", hi - lo);

    /* the step follows the type: the same walk over doubles moves 8 bytes at a time */
    double d[3] = {1.5, 2.5, 3.5};
    for (const double *q = d; q != d + 3; ++q) printf("%p: %g\n", (const void *)q, *q);
    const uintptr_t step = (uintptr_t)(d + 1) - (uintptr_t)d;
    printf("one step of a double*: %" PRIuPTR " bytes\n", step);
    printf("ints per 64 byte cache line: %zu\n", 64 / sizeof(int));
    return (hi - lo == 3 * sizeof(int) && step == sizeof(double)) ? 0 : 1;
}
