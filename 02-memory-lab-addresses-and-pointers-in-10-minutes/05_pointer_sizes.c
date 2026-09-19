/* Memory Addresses and Pointers: What Is Really Stored? - slide 5: sizeof(t*): every pointer is 8 bytes (C version of 05_pointer_sizes.cpp) */
/* Build: make 05_pointer_sizes_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

struct Big { char bytes[4096]; };

int main(void) {
    /* every object pointer is one 64 bit address on x86 64 */
    printf("%zu\n", sizeof(char *));     /* 8 */
    printf("%zu\n", sizeof(int *));      /* 8 */
    printf("%zu\n", sizeof(double *));   /* 8 */
    printf("%zu\n", sizeof(void *));     /* 8: an address with no type */
    /* the type decides how many bytes one * reads */
    double d = 1.5;
    double *pd = &d;
    printf("%zu %zu\n", sizeof(pd), sizeof(*pd));   /* 8 8 */
    char c = 'A';
    char *pc = &c;
    printf("%zu %zu\n", sizeof(pc), sizeof(*pc));   /* 8 1 */

    /* a pointer to a 4096 byte object is still one address */
    static struct Big big;               /* static storage starts zeroed */
    struct Big *pb = &big;
    printf("\nsizeof(struct Big *) = %zu, sizeof(struct Big) = %zu\n", sizeof(pb), sizeof(*pb));
    printf("*pd = %g, *pc = %c, first byte of big = %d\n", *pd, *pc, (int)pb->bytes[0]);
    printf("this build: %zu bit addresses, %zu pointers per 64 byte cache line\n",
           sizeof(void *) * 8, 64 / sizeof(void *));
    printf("a list node {int, next}: %zu bytes of data, %zu bytes of pointer\n", sizeof(int), sizeof(void *));
    return 0;
}
