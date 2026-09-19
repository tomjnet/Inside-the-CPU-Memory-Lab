/* Memory Addresses and Pointers: What Is Really Stored? - slide 10: references: an alias, not a new object (C version of 10_references.cpp) */
/* Build: make 10_references_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

/* C has no references: the closest alias is a const pointer (int *const), which can never be reseated */

/* the reference version in C++ is compiled as an address: here the address is spelled out */
static void bump(int *const n) { ++*n; }

/* the pointer version: the caller writes &, the callee has to test */
static void bump_ptr(int *n) {
    if (n) ++*n;
}

struct HoldsRef { int *const r; };   /* what a C++ int& member is under the hood */
struct HoldsPtr { int *p; };

int main(void) {
    int x = 42;
    int *const r = &x;                 /* an alias of x: the pointer itself can never change */
    printf("%d\n", r == &x);           /* 1: the same address */
    *r = 43;                           /* writes x: C needs the * a reference hides */
    int y = 7;
    *r = y;                            /* copies 7 into x: r = &y would not compile, no reseating */
    bump(&x);                          /* passes the address of x */
    printf("%d\n", x);                 /* 8 */
    /* C cannot promise "never null": test a pointer parameter, or document that it must not be NULL */

    printf("\n&x = %p, r = %p, &y = %p\n", (void *)&x, (void *)r, (void *)&y);
    printf("y is still %d: *r = y copied a value, r did not move\n", y);
    bump_ptr(&x);
    bump_ptr(NULL);                    /* legal for a pointer; a C++ reference could not be null */
    printf("after bump_ptr(&x): x = %d\n", x);
    /* stored inside an object, an alias takes the room of an address */
    printf("sizeof(struct HoldsRef) = %zu, sizeof(struct HoldsPtr) = %zu\n",
           sizeof(struct HoldsRef), sizeof(struct HoldsPtr));
    return (x == 9 && y == 7) ? 0 : 1;
}
