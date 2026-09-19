/* Stack vs Heap: Where Does Your Data Actually Live? - slide 8: lab: locals in nested calls descend (C version of 08_nested_frames.cpp) */
/* Build: make 08_nested_frames_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdint.h>
#include <stdio.h>

static int frame(int depth, uintptr_t caller);
static int (*volatile call)(int, uintptr_t) = frame;   /* never inlined */

static int frame(int depth, uintptr_t caller) {
    int local = depth;                       /* lives in this frame */
    uintptr_t here = (uintptr_t)&local;
    printf("depth %d  &local %p  below the caller by %llu B\n",
           depth, (void *)&local, (unsigned long long)(caller - here));
    int deeper = depth < 4 ? call(depth + 1, here) : 0;  /* new frame */
    return local + deeper;                   /* frame popped: rsp back */
}

int main(void) {
    int in_main = 0;
    uintptr_t top = (uintptr_t)&in_main;
    printf("main     &local %p\n", (void *)&in_main);

    int sum = call(1, top);                  /* 1 + 2 + 3 + 4: every frame returned its local */

    printf("sum of the locals on the way back: %d\n\n", sum + in_main);
    printf("Every address is lower than the one before: the stack grows down.\n");
    printf("The distance between two depths is one frame: return address, saved registers,\n");
    printf("the local and alignment padding. The first gap also holds what main keeps in its frame.\n");
    printf("The numbers change with the compiler and the flags; the direction does not.\n");
    return 0;
}
