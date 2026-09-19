/* Memory Alignment and Locality: Why Data Layout Matters - slide 5: reordering members (C version of 05_reorder.cpp) */
/* Build: make 05_reorder_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {       /* 24 bytes: 10 of them are padding */
    char   tag;        /* offset 0, then 7 bytes of padding */
    double price;      /* offset 8: must be a multiple of 8 */
    char   side;       /* offset 16, then 3 bytes of padding */
    int    qty;        /* offset 20 */
} Before;
typedef struct {       /* 16 bytes: same data, 2 bytes of padding */
    double price;      /* offset 0: biggest alignment first */
    int    qty;        /* offset 8 */
    char   tag;        /* offset 12 */
    char   side;       /* offset 13, then 2 bytes of tail padding */
} After;

int main(void) {
    const size_t data = sizeof(char) + sizeof(double) + sizeof(char) + sizeof(int);

    printf("struct Before: sizeof %zu, alignof %zu\n", sizeof(Before), alignof(Before));
    printf("  tag   at offset %zu\n", offsetof(Before, tag));
    printf("  price at offset %zu\n", offsetof(Before, price));
    printf("  side  at offset %zu\n", offsetof(Before, side));
    printf("  qty   at offset %zu\n", offsetof(Before, qty));
    printf("  data %zu bytes, padding %zu bytes\n\n", data, sizeof(Before) - data);

    printf("struct After: sizeof %zu, alignof %zu\n", sizeof(After), alignof(After));
    printf("  price at offset %zu\n", offsetof(After, price));
    printf("  qty   at offset %zu\n", offsetof(After, qty));
    printf("  tag   at offset %zu\n", offsetof(After, tag));
    printf("  side  at offset %zu\n", offsetof(After, side));
    printf("  data %zu bytes, padding %zu bytes\n\n", data, sizeof(After) - data);

    /* What the difference means for a loop: objects per 64 byte cache line and memory for a million of them. */
    const size_t line = 64, million = 1000000;
    printf("whole objects per cache line: Before %zu, After %zu\n", line / sizeof(Before), line / sizeof(After));
    printf("one million objects: Before %zu MB, After %zu MB\n", sizeof(Before) * million / 1000000,
           sizeof(After) * million / 1000000);

    /* Same members, same values: only the order of the declaration changed. */
    Before b = {'A', 101.5, 'B', 300};
    After a = {b.price, b.qty, b.tag, b.side};
    printf("same data: %c %g %c %d\n", a.tag, a.price, a.side, a.qty);
    return sizeof(After) <= sizeof(Before) ? 0 : 1;
}
