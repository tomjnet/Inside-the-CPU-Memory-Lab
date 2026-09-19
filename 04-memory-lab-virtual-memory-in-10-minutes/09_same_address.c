/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 9: same address, two processes (C version of 09_same_address.cpp) */
/* Build: make 09_same_address_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__linux__)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/* One line per process, flushed at once: the child leaves with _exit, which does not flush stdout. */
static void show(const char *who, const int *p, int v) {
    printf("  %s  &value = 0x%" PRIxPTR "  value = %d\n", who, (uintptr_t)p, v);
    fflush(stdout);
}

int value = 1;                          /* one global, one address */

#if defined(__linux__)
static void same_address(void) {
    pid_t child = fork();               /* copies the map, not the RAM */
    if (child == 0) {                   /* child: a private space */
        value = 42;                     /* copy on write: 1 new page */
        show("child ", &value, value);
        _exit(0);
    }
    waitpid(child, NULL, 0);            /* parent waits for the child */
    show("parent", &value, value);      /* same address, still 1 */
}
#endif

int main(void) {
    printf("before fork:\n");
    fflush(stdout);
    show("parent", &value, value);

#if defined(__linux__)
    printf("after fork, the child writes 42 into the global:\n");
    fflush(stdout);                     /* an unflushed buffer would be copied into the child too */
    same_address();
    printf("same virtual address, two values: two frames of RAM, one per process\n");
    printf("(the page was shared until the write: copy on write)\n");
#else
    printf("this sample needs Linux: fork() would print the same address twice, with 42 in the child\n");
    printf("and 1 in the parent, because each process has its own address space\n");
#endif
    return 0;
}
