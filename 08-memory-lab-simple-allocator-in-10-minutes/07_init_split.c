/* Build a Simple Memory Allocator: Understanding malloc and new - slide 7: init and split (C version of 07_init_split.cpp) */
/* Build: make 07_init_split_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* ---- slide 3: the arena and the interface ---- */
#define ARENA ((size_t)4096)                 /* one page, fixed forever */
#define ALIGN ((size_t)16)                   /* what malloc promises */
static _Alignas(16) unsigned char arena[ARENA];

/* round n up to a multiple of 16: 1 -> 16, 16 -> 16, 17 -> 32 */
static size_t align_up(size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

void *allocate(size_t size);                 /* NULL when it is full */
void  deallocate(void *ptr);                 /* no size: we store it */

/* ---- slide 5: a header in front of every block ---- */
typedef struct {                  /* header in front of every payload */
    size_t size;                  /* payload bytes, multiple of 16 */
    bool   free;                  /* may allocate() hand it out? */
} Block;                          /* 16 bytes with padding on x86 64 */
static Block *first(void) { return (Block *)(void *)arena; }
static unsigned char *payload(Block *b) {    /* bytes after the header */
    return (unsigned char *)b + ALIGN;
}
static Block *next(Block *b) {               /* header + payload = next */
    unsigned char *p = payload(b) + b->size;
    return p < arena + ARENA ? (Block *)(void *)p : NULL;
}

/* ---- slide 7: init and split ---- */
/* C has no placement new: a cast pointer names the bytes, then the fields are written */
static void write_header(unsigned char *at, size_t size, bool is_free) {
    Block *b = (Block *)(void *)at;
    b->size = size;
    b->free = is_free;
}
static void init(void) {                     /* one free block: 4080 */
    write_header(arena, ARENA - ALIGN, true);
}
static void split(Block *b, size_t need) {   /* O(1): write one header */
    if (b->size < need + 2 * ALIGN) return;  /* rest too small: keep it */
    write_header(payload(b) + need, b->size - need - ALIGN, true);
    b->size = need;                          /* b shrinks, rest is free */
}

/* ---- slide 6: allocate: first fit ---- */
void *allocate(size_t size) {                /* O(blocks): walk them all */
    size_t need = align_up(size ? size : 1);
    for (Block *b = first(); b; b = next(b)) {
        if (!b->free || b->size < need) continue;   /* first fit */
        split(b, need);                      /* give back what is left */
        b->free = false;
        return payload(b);                   /* 16 bytes past the header */
    }
    return NULL;                             /* nothing big enough */
}

/* ---- slide 9: print the block map ---- */
static void print_map(const char *what) {    /* one line per operation */
    printf("%-20s|", what);
    for (Block *b = first(); b; b = next(b))
        printf("%s%zu |", b->free ? " free " : " USED ", b->size);
    printf("\n");
}
/* init()              | free 4080 |
   a = allocate(100)   | USED 112 | free 3952 |
   b = allocate(200)   | USED 112 | USED 208 | free 3728 |
   deallocate(a)       | free 112 | USED 208 | free 3728 |
   deallocate(b)       | free 4080 | */

int main(void) {
    init();
    print_map("init()");
    void *a = allocate(100);
    print_map("a = allocate(100)");
    void *b = allocate(200);
    print_map("b = allocate(200)");
    printf("4080 = 112 + 16 + 3952, then 3952 = 208 + 16 + 3728: every split costs one header\n\n");
    bool ok = a != NULL && b != NULL && next(next(first()))->size == 3728;

    /* the case where split() returns early: the rest could not hold a header plus 16 bytes */
    init();
    void *whole = allocate(4060);             /* rounds up to 4064, only 16 bytes would be left */
    print_map("allocate(4060)");
    printf("asked for 4060, the block holds %zu: no split, the caller gets the extra\n", first()->size);
    ok = ok && whole != NULL && first()->size == 4080 && next(first()) == NULL;
    return ok ? 0 : 1;
}
