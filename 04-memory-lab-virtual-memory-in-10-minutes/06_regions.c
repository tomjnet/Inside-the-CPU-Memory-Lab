/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 6: addresses by region (C version of 06_regions.cpp) */
/* Build: make 06_regions_c */
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

/* Prints one virtual address. A function pointer cannot become a void*, but any pointer can become an integer. */
static void show(const char *region, uintptr_t p) {
    printf("  %s  0x%012" PRIxPTR "\n", region, p);
}

int global_counter = 1;                     /* data: lives all run long */

void print_regions(void);

void print_regions(void) {
    int local = 2;                          /* stack: top of the space */
    int *heap = malloc(sizeof *heap);       /* heap: above the data (C has no unique_ptr: free below) */
    if (heap != NULL) *heap = 3;
    show("text ", (uintptr_t)&print_regions);   /* code: read only pages */
    show("data ", (uintptr_t)&global_counter);
    show("heap ", (uintptr_t)heap);
    show("stack", (uintptr_t)&local);       /* every one is virtual: */
    free(heap);                             /* no RAM position in sight */
}

/* The distance between two addresses, in the unit that reads best. */
static void gap(const char *label, uintptr_t a, uintptr_t b) {
    const double bytes = (double)(a > b ? a - b : b - a);
    const double kib = 1024.0, mib = kib * 1024.0, gib = mib * 1024.0;
    printf("  %s  ", label);
    if (bytes >= gib) printf("%.1f GiB\n", bytes / gib);
    else if (bytes >= mib) printf("%.1f MiB\n", bytes / mib);
    else printf("%.1f KiB\n", bytes / kib);
}

int main(void) {
    printf("one object per region, virtual addresses:\n");
    print_regions();

    int local = 0;
    int *heap = malloc(sizeof *heap);
    if (heap == NULL) {
        printf("malloc refused 4 bytes: nothing to measure\n");
        return 0;
    }
    *heap = 0;
    const uintptr_t text_at = (uintptr_t)&print_regions;
    const uintptr_t data_at = (uintptr_t)&global_counter;
    const uintptr_t heap_at = (uintptr_t)heap;
    const uintptr_t stack_at = (uintptr_t)&local;

    printf("\ngaps in this run:\n");
    gap("text  to data ", text_at, data_at);
    gap("data  to heap ", data_at, heap_at);
    gap("heap  to stack", heap_at, stack_at);

#if defined(__linux__)
    printf("\nLinux order, low to high: text, data, heap, (mmap region), stack: %s\n",
           (text_at < data_at && data_at < heap_at && heap_at < stack_at) ? "yes" : "not in this build");
#else
    printf("\nthis is not Linux: the regions exist too, but the order and the gaps differ\n");
#endif
    printf("run it again: address space layout randomization moves every region\n");
    free(heap);
    return 0;
}
