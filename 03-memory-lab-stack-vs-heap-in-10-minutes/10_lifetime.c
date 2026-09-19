/* Stack vs Heap: Where Does Your Data Actually Live? - slide 10: lifetime and ownership (C version of 10_lifetime.cpp) */
/* Build: make 10_lifetime_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdio.h>
#include <stdlib.h>

static int *dangling(void) {
    int local = 42;          /* dies at the closing brace */
    printf("  local = %d at %p: valid only inside this call\n", local, (void *)&local);
    /* return &local;        frame popped: the pointer dangles */
    /* (kept as a comment: the compiler warns, and reading through it is undefined behaviour) */
    return NULL;
}
static int *make_answer(void) {
    int *p = malloc(sizeof *p);   /* lives until free, not until the brace */
    if (p != NULL) *p = 42;
    return p;                     /* the caller owns it now: who frees? */
}
/* forget the free: a leak.  free twice: undefined behaviour. */
/* use after free: undefined behaviour, often silent. */

int main(void) {
    printf("stack: the lifetime is the scope\n");
    int *gone = dangling();
    printf("  dangling() returned %s: the honest answer, the local no longer exists\n\n",
           gone == NULL ? "NULL" : "a pointer");

    printf("heap: the lifetime is yours\n");
    int *answer = make_answer();
    if (answer == NULL) return 1;
    printf("  make_answer() returned %p holding %d: still alive after the function returned\n",
           (void *)answer, *answer);
    free(answer);            /* exactly one free, by the owner: main */
    answer = NULL;           /* never read it again: use after free is undefined */
    printf("  freed once by its owner, pointer reset to %s\n\n", answer == NULL ? "NULL" : "?");

    printf("No free: the block leaks until the process ends. Two frees or a read after\n");
    printf("the free: undefined behaviour. Those lines are described, never executed.\n");
    return 0;
}
