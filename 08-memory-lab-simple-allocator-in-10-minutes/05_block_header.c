/* Build a Simple Memory Allocator: Understanding malloc and new - slide 5: a header in front of every block (C version of 05_block_header.cpp) */
/* Build: make 05_block_header_c */
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

static size_t offset_of(void *p) {           /* distance from the start of the arena, easier to read than an address */
    return (size_t)((unsigned char *)p - arena);
}

int main(void) {
    _Static_assert(sizeof(Block) <= 16, "the header must fit in 16 bytes");
    printf("sizeof(Block) = %zu, _Alignof(Block) = %zu (size_t + bool + padding)\n",
           sizeof(Block), (size_t)_Alignof(Block));

    init();
    Block *b = first();
    printf("after init(): header at arena + %zu, payload at arena + %zu, size %zu, free %d\n",
           offset_of(b), offset_of(payload(b)), b->size, (int)b->free);
    bool ok = offset_of(payload(b)) == ALIGN && b->size == ARENA - ALIGN && next(b) == NULL;
    printf("next(first()) = NULL: one block covers the whole arena\n\n");

    split(b, align_up(100));                  /* 112: what allocate(100) will do on the next slide */
    Block *rest = next(b);
    printf("after split(first(), 112):\n");
    printf("  block 0: header at arena + %zu, size %zu\n", offset_of(b), b->size);
    printf("  block 1: header at arena + %zu, size %zu\n", offset_of(rest), rest->size);
    printf("  header + 16 + size = next header: 0 + 16 + 112 = %zu\n", offset_of(rest));
    ok = ok && offset_of(rest) == 128 && rest->size == 3952 && rest->free && next(rest) == NULL;

    /* the way back, which deallocate uses: payload pointer minus 16 is the header */
    void *user = payload(rest);
    Block *back = (Block *)(void *)((unsigned char *)user - ALIGN);
    printf("user pointer arena + %zu minus 16 -> header with size %zu\n", offset_of(user), back->size);
    ok = ok && back == rest;
    printf("%s", ok ? "header arithmetic checks out\n" : "HEADER ARITHMETIC IS WRONG\n");
    return ok ? 0 : 1;
}
