/* Computer Memory in 5 Minutes: From Registers to RAM - slide 5: three addresses: global, local and heap (C version of 05_three_addresses.cpp) */
/* Build: make 05_three_addresses_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int global_counter = 7;            /* static storage: lives all run */

static int show(void) {
    int local = 1;                 /* stack: moves one register */
    int *heap = malloc(sizeof *heap); /* heap: a call into the allocator */
    if (heap == NULL) return 1;
    *heap = 2;
    printf("%p\n%p\n%p\n", (void *)&global_counter, (void *)&local,
           (void *)heap);          /* three numbers, three regions */
    free(heap);                    /* the heap needs a free */
    return 0;
}

/* The same three objects again, as plain numbers, so the distances between the regions can be printed. */
static double mib_between(uintptr_t a, uintptr_t b) {
    uintptr_t d = a > b ? a - b : b - a;
    return (double)d / (1024.0 * 1024.0);
}

int main(void) {
    printf("global, local, heap (this machine, this run):\n");
    if (show() != 0) return 1;

    int local = 1;
    int *heap = malloc(sizeof *heap);
    if (heap == NULL) return 1;
    *heap = 2;
    uintptr_t g = (uintptr_t)&global_counter;
    uintptr_t l = (uintptr_t)&local;
    uintptr_t h = (uintptr_t)heap;
    printf("\nvalues: %d %d %d\n", global_counter, local, *heap);
    printf("global to heap : %g MiB apart\n", mib_between(g, h));
    printf("heap to stack  : %g MiB apart\n", mib_between(h, l));
    printf("global to stack: %g MiB apart\n", mib_between(g, l));
    printf("highest address: %s\n",
           (l > g && l > h) ? "the stack (typical on Linux)" : "not the stack on this system");
    printf("run it twice: the numbers change (address space layout randomization), the regions do not\n");
    free(heap);
    return 0;
}
