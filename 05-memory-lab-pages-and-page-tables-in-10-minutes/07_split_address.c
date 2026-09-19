/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 7: split an address in c++ (C version of 07_split_address.cpp) */
/* Build: make 07_split_address_c */
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

/* x86 64, 4 level paging: 9 + 9 + 9 + 9 + 12 = 48 bits.
   C has no constexpr function, so a macro keeps the index usable in _Static_assert. */
#define INDEX(va, level) ((((uint64_t)(va)) >> (12 + 9 * (level))) & 0x1FF)   /* 9 bits: 0 to 511 */

_Static_assert(INDEX(0x00007f3a5c2d1e48ULL, 3) == 254 && INDEX(0x00007f3a5c2d1e48ULL, 0) == 209,
               "the numbers of the slide");

static int global_value = 7;

static void split(const char *label, uint64_t a) {
    printf("%-10s 0x%016" PRIx64 "  PML4 %3" PRIu64 "  PDPT %3" PRIu64 "  PD %3" PRIu64 "  PT %3" PRIu64
           "  offset 0x%" PRIx64 "\n",
           label, a, INDEX(a, 3), INDEX(a, 2), INDEX(a, 1), INDEX(a, 0), a & 0xFFF);
}

int main(void) {
    uint64_t va = 0x00007f3a5c2d1e48ULL;
    uint64_t pml4 = INDEX(va, 3);        /* bits 47 to 39: 254 */
    uint64_t pdpt = INDEX(va, 2);        /* bits 38 to 30: 233 */
    uint64_t pd   = INDEX(va, 1);        /* bits 29 to 21: 225 */
    uint64_t pt   = INDEX(va, 0);        /* bits 20 to 12: 209 */
    uint64_t offset = va & 0xFFF;        /* bits 11 to 0: 0xe48 */
    /* bits 63 to 48 copy bit 47: the canonical form */

    printf("the address of the slide\n");
    split("example", va);

    /* put the fields back together: nothing was lost, the split is only shifts and masks */
    uint64_t again = (pml4 << 39) | (pdpt << 30) | (pd << 21) | (pt << 12) | offset;
    printf("rebuilt    0x%016" PRIx64 "%s\n", again, again == va ? "  (same address)" : "  (MISMATCH)");
    if (again != va) return 1;

    int local_value = 1;
    int *heap_value = malloc(sizeof *heap_value);   /* no unique_ptr in C: free it by hand below */
    if (heap_value == NULL) {
        printf("malloc failed\n");
        return 1;
    }
    *heap_value = 2;
    printf("\nyour own addresses (they change on every run: the kernel randomizes the layout)\n");
    split("local", (uint64_t)(uintptr_t)&local_value);
    split("next int", (uint64_t)(uintptr_t)&local_value + sizeof(int));
    split("global", (uint64_t)(uintptr_t)&global_value);
    split("heap", (uint64_t)(uintptr_t)heap_value);
    printf("\nlocal and the int after it share the four indexes (unless they straddle a page): only the offset\n"
           "moves. Stack, globals and heap sit far apart, so they usually differ at the PML4 or PDPT index.\n");
    printf("values: %d\n", local_value + global_value + *heap_value);
    free(heap_value);
    return 0;
}
