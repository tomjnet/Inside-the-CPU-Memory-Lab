/* Build a Simple Memory Allocator: Understanding malloc and new - slide 10: check every pointer (C version of 10_check_pointers.cpp) */
/* Build: make 10_check_pointers_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

/* ---- slide 8: deallocate and coalesce ---- */
void deallocate(void *ptr) {                 /* O(blocks): one pass */
    if (!ptr) return;                        /* like free(NULL) */
    unsigned char *bytes = ptr;
    ((Block *)(void *)(bytes - ALIGN))->free = true;       /* back 16 */
    for (Block *b = first(); b; b = next(b))               /* coalesce */
        while (b->free && next(b) && next(b)->free)
            b->size += ALIGN + next(b)->size;    /* absorb the neighbour */
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

/* ---- slide 10: check every pointer ---- */
static bool valid(void *ptr) {               /* what every block obeys */
    uintptr_t a  = (uintptr_t)ptr;
    uintptr_t lo = (uintptr_t)arena;
    return a % ALIGN == 0                    /* aligned to 16 */
        && a >= lo + ALIGN                   /* after the first header */
        && a <  lo + ARENA;                  /* inside the arena */
}
/* stress: 1000 random operations, sizes from 1 to 256 bytes:
   every pointer valid, every block keeps its fill byte (no overlap),
   and after freeing everything the map is one free block of 4080 */

/* every payload plus 16 bytes per header must add up to the arena: true after any operation */
static bool adds_up(void) {
    size_t total = 0;
    for (Block *b = first(); b; b = next(b)) total += ALIGN + b->size;
    return total == ARENA;
}

typedef struct { unsigned char *ptr; size_t size; unsigned char fill; } Live;

static bool intact(const Live *l) {          /* does the block still hold the byte we wrote into it? */
    for (size_t i = 0; i < l->size; ++i)
        if (l->ptr[i] != l->fill) return false;
    return true;
}

/* C has no std::mt19937: a fixed-seed xorshift64 gives the same run on every machine,
   but a different sequence than the C++ program, so the counts below differ from its run */
static uint64_t rng_state = 8;               /* fixed seed, never zero */
static uint64_t rng(void) {
    uint64_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

int main(void) {
    init();
    static Live live[32];                     /* at most 32 blocks alive at the same time, all zero */
    int allocs = 0, frees = 0, full = 0, bad_pointer = 0, overlap = 0, broken = 0;

    for (int op = 0; op < 1000; ++op) {
        Live *slot = &live[rng() % 32];
        if (slot->ptr == NULL) {
            size_t size = (size_t)(rng() % 256 + 1);
            void *p = allocate(size);
            if (p == NULL) { ++full; continue; }           /* the arena may be full: that is allowed */
            if (!valid(p)) ++bad_pointer;
            slot->ptr = p;
            slot->size = size;
            slot->fill = (unsigned char)(op % 251 + 1);
            memset(slot->ptr, slot->fill, slot->size);
            ++allocs;
        } else {
            if (!intact(slot)) ++overlap;                  /* someone else wrote into our bytes */
            deallocate(slot->ptr);
            slot->ptr = NULL;
            slot->size = 0;
            slot->fill = 0;
            ++frees;
        }
        if (!adds_up()) ++broken;
    }
    print_map("after 1000 ops");

    for (size_t i = 0; i < 32; ++i) {
        Live *slot = &live[i];
        if (slot->ptr == NULL) continue;
        if (!intact(slot)) ++overlap;
        deallocate(slot->ptr);
        ++frees;
    }
    print_map("all freed");

    bool one_block = first()->free && first()->size == ARENA - ALIGN && next(first()) == NULL;
    printf("\n%d allocations, %d frees, %d requests refused (arena full)\n", allocs, frees, full);
    printf("pointers not aligned or outside the arena: %d\n", bad_pointer);
    printf("blocks that lost their fill byte (overlap): %d\n", overlap);
    printf("operations after which the sizes did not add up: %d\n", broken);
    printf("one free block of 4080 at the end: %s\n", one_block ? "yes" : "NO");
    bool ok = bad_pointer == 0 && overlap == 0 && broken == 0 && one_block && allocs == frees && !valid(arena);
    return ok ? 0 : 1;
}
