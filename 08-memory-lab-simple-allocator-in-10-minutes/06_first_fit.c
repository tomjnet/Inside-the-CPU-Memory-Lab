/* Build a Simple Memory Allocator: Understanding malloc and new - slide 6: allocate: first fit (C version of 06_first_fit.cpp) */
/* Build: make 06_first_fit_c */
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

static long long offset_of(void *p) {        /* distance from the start of the arena, -1 for NULL */
    return p ? (long long)((unsigned char *)p - arena) : -1;
}

int main(void) {
    init();
    void *a = allocate(100);
    void *b = allocate(200);
    void *c = allocate(0);                    /* like malloc(0) here: a real block of 16 bytes */
    void *d = allocate(5000);                 /* bigger than the arena */
    printf("a = allocate(100)  -> arena + %lld\n", offset_of(a));
    printf("b = allocate(200)  -> arena + %lld   (16 + 112 + 16)\n", offset_of(b));
    printf("c = allocate(0)    -> arena + %lld   (144 + 208 + 16)\n", offset_of(c));
    printf("d = allocate(5000) -> %s\n\n", d ? "a block?" : "NULL: nothing big enough");

    printf("the walk first fit does, header by header:\n");
    int visited = 0;
    for (Block *blk = first(); blk; blk = next(blk)) {
        printf("  block %d: %s %zu\n", visited++, blk->free ? "free" : "USED", blk->size);
    }
    printf("a fourth allocate reads %d headers before it finds the free block: O(blocks)\n", visited);

    bool ok = offset_of(a) == 16 && offset_of(b) == 144 && offset_of(c) == 368 && d == NULL && visited == 4;
    printf("%s", ok ? "first fit returned the expected addresses\n" : "UNEXPECTED ADDRESSES\n");
    return ok ? 0 : 1;
}
