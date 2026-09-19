/* Memory Addresses and Pointers: What Is Really Stored? - slide 9: nullptr: the address of nothing (C version of 09_nullptr.cpp) */
/* Build: make 09_nullptr_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* C17 has no nullptr keyword: NULL, the null pointer constant, points at no object */
static const int *find(const int *p, size_t n, int key) {
    for (size_t i = 0; i < n; ++i)
        if (p[i] == key) return p + i;     /* address of the match */
    return NULL;                           /* not found */
}

int main(void) {
    int a[4] = {10, 20, 30, 40};
    const int *hit = find(a, 4, 30);
    if (hit) printf("%td\n", hit - a);         /* 2: test before the * */
    const int *miss = find(a, 4, 99);
    printf("%d\n", miss == NULL);              /* 1: never dereference */

    /* observed only, never dereferenced: the number inside a null pointer */
    printf("\nhit  = %p, *hit = %d\n", (const void *)hit, hit ? *hit : -1);
    printf("miss as a number = %" PRIuPTR "\n", (uintptr_t)miss);
    /* printf("%d", *miss);   undefined behaviour: on Linux typically a segmentation fault, page 0 is never mapped */
    printf("sizeof((void *)0) = %zu: C17 has no nullptr type, NULL is a macro for a null pointer constant\n",
           sizeof((void *)0));
    return (hit == a + 2 && miss == NULL) ? 0 : 1;
}
