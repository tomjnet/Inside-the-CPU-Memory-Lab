/* Build a Simple Memory Allocator: Understanding malloc and new - slide 3: the arena and the interface (C version of 03_arena_interface.cpp) */
/* Build: make 03_arena_interface_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* ---- slide 3: the arena and the interface ---- */
#define ARENA ((size_t)4096)                 /* one page, fixed forever */
#define ALIGN ((size_t)16)                   /* what malloc promises */
static _Alignas(16) unsigned char arena[ARENA];

/* round n up to a multiple of 16: 1 -> 16, 16 -> 16, 17 -> 32 */
static size_t align_up(size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

/* MSVC's C headers have no max_align_t; its C++ one is double, so ask for that instead */
#if defined(_MSC_VER) && !defined(__clang__)
#define MAX_ALIGN_T double
#else
#define MAX_ALIGN_T max_align_t
#endif

void *allocate(size_t size);                 /* NULL when it is full */
void  deallocate(void *ptr);                 /* no size: we store it */

/* allocate() and deallocate() are only declared here: slide 4 defines the bump version,
   slides 6 and 8 the free list version. This program shows the arena and the rounding. */

int main(void) {
    uintptr_t where = (uintptr_t)arena;
    printf("arena: %zu bytes of static storage at %p\n", ARENA, (void *)arena);
    printf("arena address %% 16 = %zu (_Alignas(16) on the array)\n\n", (size_t)(where % ALIGN));

    printf("align_up rounds every request to a multiple of %zu:\n", ALIGN);
    const size_t requests[] = { 1, 15, 16, 17, 24, 100, 200, 1000 };
    bool ok = where % ALIGN == 0;
    for (size_t i = 0; i < sizeof requests / sizeof requests[0]; ++i) {
        size_t n = requests[i];
        size_t r = align_up(n);
        printf("  align_up(%zu) = %zu  (%zu bytes of padding)\n", n, r, r - n);
        if (r % ALIGN != 0 || r < n || r - n >= ALIGN) ok = false;
    }
    printf("\n_Alignof(max_align_t) with this compiler: %zu"
           " (16 with gcc on x86 64; MSVC says 8, yet its malloc aligns to 16 too)\n",
           (size_t)_Alignof(MAX_ALIGN_T));
    printf("%s", ok ? "every rounded size is a multiple of 16 and wastes less than 16 bytes\n"
                    : "ROUNDING IS WRONG\n");
    return ok ? 0 : 1;
}
