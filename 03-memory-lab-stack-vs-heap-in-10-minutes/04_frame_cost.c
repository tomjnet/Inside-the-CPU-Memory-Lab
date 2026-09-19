/* Stack vs Heap: Where Does Your Data Actually Live? - slide 4: a stack allocation is one subtract (C version of 04_frame_cost.cpp) */
/* Build: make 04_frame_cost_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdint.h>
#include <stdio.h>

typedef struct { int x; int y; } Point;

static int sum_three(int a, int b, int c) {
    int   total = a + b + c;       /* 4 B, or just a register */
    char  buf[64];                 /* 64 B in the same frame */
    Point p = { a, b };            /* 8 B in the same frame */
    /* prologue: sub rsp, N    one subtract reserves all of it */
    /* epilogue: add rsp, N    one add releases all of it */
    /* no search, no lock, no system call, no header: O(1) */
    buf[0] = (char)(total + p.x);
    return buf[0] + p.y;
}

static uintptr_t at(const void *q) { return (uintptr_t)q; }

/* The same three locals, but their addresses are printed, so the compiler must keep them in memory.
   They all sit inside one frame: the distance between the lowest and the highest is about their total size. */
static void show_frame(int a, int b, int c) {
    int   total = a + b + c;
    char  buf[64];
    Point p = { a, b };
    buf[0] = (char)(total + p.x);

    uintptr_t lo = at(&total);
    uintptr_t hi = at(&total) + sizeof total;
    if (at(buf) < lo) lo = at(buf);
    if (at(&p) < lo) lo = at(&p);
    if (at(buf) + sizeof buf > hi) hi = at(buf) + sizeof buf;
    if (at(&p) + sizeof p > hi) hi = at(&p) + sizeof p;

    printf("  &total %p  (%zu B)\n", (void *)&total, sizeof total);
    printf("  buf    %p  (%zu B, buf[0] = %d)\n", (void *)buf, sizeof buf, (int)buf[0]);
    printf("  &p     %p  (%zu B, p.y = %d)\n", (void *)&p, sizeof p, p.y);
    printf("  sum of the sizes: %zu B\n", sizeof total + sizeof buf + sizeof p);
    printf("  span in the frame: %zu B (sizes plus alignment padding)\n", (size_t)(hi - lo));
}

int main(void) {
    printf("sum_three(1, 2, 3) = %d\n\n", sum_three(1, 2, 3));

    printf("three locals, one frame, one subtract on rsp:\n");
    show_frame(1, 2, 3);

    printf("\nNo allocator ran: the frame was reserved by moving the stack pointer once.\n");
    printf("See it yourself: gcc -O2 -S -masm=intel 04_frame_cost.c -o - and look for sub rsp.\n");
    return 0;
}
