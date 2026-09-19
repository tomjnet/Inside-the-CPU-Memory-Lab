/* Memory Addresses and Pointers: What Is Really Stored? - slide 8: arrays decay to pointers (C version of 08_array_decay.cpp) */
/* Build: make 08_array_decay_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdio.h>

/* an array parameter is a pointer: the length is gone */
static long long sum(const int *p, size_t n) {
    long long s = 0;
    for (size_t i = 0; i < n; ++i) s += p[i];   /* *(p + i) */
    return s;
}

/* C has no std::span: a small struct carries the pointer and the length together */
typedef struct { const int *data; size_t len; } IntSpan;

static long long sum_span(IntSpan v) {
    long long s = 0;
    for (size_t i = 0; i < v.len; ++i) s += v.data[i];
    return s;
}

int main(void) {
    int a[4] = {10, 20, 30, 40};
    printf("%zu\n", sizeof a);          /* 16: still an array here */
    const int *p = a;                   /* decay: the address of a[0] */
    printf("%zu\n", sizeof p);          /* 8: only the address is left */
    printf("%lld\n", sum(a, 4));        /* so pass the length yourself */

    printf("\np == &a[0]: %d\n", p == &a[0]);
    printf("a[2] = %d, *(a + 2) = %d, p[2] = %d\n", a[2], *(a + 2), p[2]);
    printf("elements from sizeof: %zu (only where a is still an array)\n", sizeof a / sizeof a[0]);
    const IntSpan span = {a, sizeof a / sizeof a[0]};
    printf("sum through IntSpan: %lld, sizeof(IntSpan) = %zu\n", sum_span(span), sizeof(IntSpan));
    return (sum(a, 4) == 100 && sum_span(span) == 100) ? 0 : 1;
}
