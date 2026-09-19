/* Build a Simple Memory Allocator: Understanding malloc and new - slide 8: deallocate and coalesce (C version of 08_deallocate_coalesce.cpp) */
/* Build: make 08_deallocate_coalesce_c */
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

int main(void) {
    init();
    void *a = allocate(100);
    void *b = allocate(200);
    print_map("a and b allocated");

    deallocate(a);
    print_map("deallocate(a)");               /* b is still USED: nothing to merge */
    bool ok = first()->free && first()->size == 112 && !next(first())->free;

    deallocate(b);
    print_map("deallocate(b)");               /* three free neighbours become one block */
    ok = ok && first()->free && first()->size == ARENA - ALIGN && next(first()) == NULL;
    printf("112 + 16 + 208 + 16 + 3728 = %d\n\n", 112 + 16 + 208 + 16 + 3728);

    deallocate(NULL);                         /* does nothing, like free(NULL) */
    print_map("deallocate(NULL)");

    /* free in the other order: the merge must come out the same */
    a = allocate(100);
    b = allocate(200);
    deallocate(b);
    print_map("deallocate(b) first");
    deallocate(a);
    print_map("then deallocate(a)");
    ok = ok && first()->size == ARENA - ALIGN && next(first()) == NULL;
    printf("%s", ok ? "the arena is one free block again\n" : "COALESCE IS WRONG\n");
    return ok ? 0 : 1;
}
