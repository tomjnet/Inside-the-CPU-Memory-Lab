/* Memory Addresses and Pointers: What Is Really Stored? - slide 4: take an address, follow it (C version of 04_address_and_deref.cpp) */
/* Build: make 04_address_and_deref_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

int main(void) {
    int x = 42;
    int *p = &x;                    /* & takes the address: no memory read */
    printf("%p\n", (void *)p);      /* the address: 0x7ffd1c2a4b3c or similar */
    printf("%d\n", *p);             /* * follows it: one 4 byte read, 42 */
    *p = 43;                        /* one 4 byte write through the pointer */
    printf("%d\n", x);              /* 43: x and *p are the same bytes */
    int **pp = &p;                  /* a pointer has an address too */
    printf("%d\n", **pp);           /* two reads: first p, then x */

    /* the same facts, labelled: the pointer is a variable with its own address and size */
    printf("\n&x  (where x lives)    = %p\n", (void *)&x);
    printf("p   (the number in p)  = %p\n", (void *)p);
    printf("&p  (where p lives)    = %p\n", (void *)&p);
    printf("pp  (the number in pp) = %p\n", (void *)pp);
    printf("sizeof x = %zu, sizeof p = %zu, sizeof pp = %zu\n", sizeof x, sizeof p, sizeof pp);
    printf("p == &x: %d, *pp == p: %d\n", p == &x, *pp == p);
    printf("the addresses change on every run: the operating system randomizes the layout\n");
    return (p == &x && *pp == p && x == 43) ? 0 : 1;
}
