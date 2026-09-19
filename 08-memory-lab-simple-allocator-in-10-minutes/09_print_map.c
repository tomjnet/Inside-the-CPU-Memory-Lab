/* Build a Simple Memory Allocator: Understanding malloc and new - slide 9: print the block map (C version of 09_print_map.cpp) */
/* Build: make 09_print_map_c */
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

/* every payload plus 16 bytes per header must add up to the arena: true after any operation */
static bool adds_up(void) {
    size_t total = 0;
    for (Block *b = first(); b; b = next(b)) total += ALIGN + b->size;
    return total == ARENA;
}

int main(void) {
    bool ok = true;
    init();
    print_map("init()");
    ok = ok && adds_up();
    void *a = allocate(100);
    print_map("a = allocate(100)");
    ok = ok && adds_up();
    void *b = allocate(200);
    print_map("b = allocate(200)");
    ok = ok && adds_up();
    deallocate(a);
    print_map("deallocate(a)");
    ok = ok && adds_up();
    deallocate(b);
    print_map("deallocate(b)");
    ok = ok && adds_up();

    printf("\ninvariant, checked after every line: payloads + 16 per header = %zu: %s\n",
           ARENA, ok ? "held" : "BROKEN");
    printf("the sizes above are the same on every machine; only the addresses differ\n");
    return ok ? 0 : 1;
}
