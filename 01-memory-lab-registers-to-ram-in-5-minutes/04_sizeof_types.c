/* Computer Memory in 5 Minutes: From Registers to RAM - slide 4: bits, bytes and the address as a number (C version of 04_sizeof_types.cpp) */
/* Build: make 04_sizeof_types_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    /* sizeof is answered by the compiler: zero cost at run time */
    printf("bits in a byte %d\n", CHAR_BIT);                  /* 8 */
    printf("char      %zu\n", sizeof(char));                  /* 1 */
    printf("short     %zu\n", sizeof(short));                 /* 2 */
    printf("int       %zu\n", sizeof(int));                   /* 4 */
    printf("long      %zu\n", sizeof(long));                  /* 8, Windows 4 */
    printf("long long %zu\n", sizeof(long long));             /* 8 */
    printf("double    %zu\n", sizeof(double));                /* 8 */
    /* 8 bytes = 64 bits: a pointer holds an address, a plain number */
    printf("void*     %zu\n", sizeof(void *));                /* 8 */

    /* The compiler really answers it: these are checked before the program exists. */
    _Static_assert(sizeof(char) == 1, "a char is one byte by definition");
    _Static_assert(sizeof(void *) * CHAR_BIT == 64, "this lab assumes a 64 bit build");

    /* Memory is a row of bytes and the address is the index: four neighbours, four consecutive numbers. */
    static unsigned char row[4] = {10, 20, 30, 40};
    printf("\nfour bytes in a row (this machine, this run):\n");
    for (size_t i = 0; i < 4; ++i) {
        uintptr_t address = (uintptr_t)&row[i];
        printf("  row[%zu] = %d  at %p  = %" PRIuPTR " as a plain number\n",
               i, (int)row[i], (const void *)&row[i], address);
    }
    uintptr_t first = (uintptr_t)&row[0];
    uintptr_t last = (uintptr_t)&row[3];
    printf("last minus first = %" PRIuPTR " bytes: the address counts bytes\n", last - first);
    return (last - first == 3) ? 0 : 1;
}
