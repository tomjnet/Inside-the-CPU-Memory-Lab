/* Memory Addresses and Pointers: What Is Really Stored? - slide 11: dangling pointers: the address outlives the object (C version of 11_dangling.cpp) */
/* Build: make 11_dangling_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *p = malloc(sizeof *p);
    if (p == NULL) return 1;
    *p = 7;
    int *q = p;                        /* two pointers, one object */
    printf("%d\n", p == q);            /* 1: the same address */
    const uintptr_t old_address = (uintptr_t)q;   /* saved as a plain number, before the free */
    free(p);                           /* the object ends, the number stays */
    p = NULL;                          /* p can be tested now */
    /* q still holds the old address: a dangling pointer */
    /* *q = 1;                            undefined behaviour: never run */
    /* the same bug: returning the address of a local variable */
    /* C has no std::unique_ptr and no destructor: one named owner, and exactly one free on every path */
    int *owner = malloc(sizeof *owner);
    if (owner == NULL) return 1;
    *owner = 7;
    printf("%d\n", *owner);

    /* q is never read again: a dangling pointer is not even looked at, so we print the saved number */
    printf("\np == NULL: %d\n", p == NULL);
    printf("the number q still holds: 0x%" PRIxPTR " (no object lives there now)\n", old_address);
    /* int *make(void) { int local = 7; return &local; }   invalid: the frame of make() is gone after the return */
    int *moved = owner;                /* a move by hand: copy the address, then clear the old owner */
    owner = NULL;
    printf("moved holds %d, sizeof(int *) = %zu: one address, the owner is you\n", *moved, sizeof moved);
    free(moved);                       /* the only owner frees once; free(owner) would be free(NULL), a no op */
    free(owner);
    return 0;
}
