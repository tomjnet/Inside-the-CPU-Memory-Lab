/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 8: a page walk as a model (C version of 08_walk_model.cpp) */
/* Build: make 08_walk_model_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Table;
typedef struct {
    bool present;
    uint64_t frame;                 /* used by the last level */
    struct Table *next;             /* used by the three upper levels */
} Entry;
typedef struct Table { Entry e[512]; } Table;

static uint64_t index_of(uint64_t va, int level) {
    return (va >> (12 + 9 * level)) & 0x1FF;
}

/* four table reads per translation: that is what the TLB saves.
   C has no exceptions: a page fault is a return value of false, and *pa is only written on success. */
static bool translate(const Table *pml4, uint64_t va, uint64_t *pa) {
    const Table *t = pml4;                            /* CR3 points here */
    for (int level = 3; level > 0; --level) {         /* PML4, PDPT, PD */
        const Entry *e = &t->e[index_of(va, level)];  /* one memory read */
        if (!e->present) return false;                /* no mapping: page fault */
        t = e->next;
    }
    const Entry *pte = &t->e[index_of(va, 0)];        /* PT: the last read */
    if (!pte->present) return false;
    *pa = (pte->frame << 12) | (va & 0xFFF);          /* frame + offset */
    return true;
}

/* The kernel side of the model: create the missing tables on the way down, then fill the last entry.
   No vector of unique_ptr in C: a fixed array owns the tables and free_tables() releases them. */
#define MAX_TABLES 16
static Table *g_tables[MAX_TABLES];
static size_t g_table_count = 0;

static Table *new_table(void) {
    if (g_table_count == MAX_TABLES) return NULL;
    Table *t = calloc(1, sizeof *t);                  /* calloc: every entry starts not present */
    if (t != NULL) g_tables[g_table_count++] = t;
    return t;
}

static void free_tables(void) {
    for (size_t i = 0; i < g_table_count; ++i) free(g_tables[i]);
    g_table_count = 0;
}

static bool map_page(Table *pml4, uint64_t va, uint64_t frame) {
    Table *t = pml4;
    for (int level = 3; level > 0; --level) {
        Entry *e = &t->e[index_of(va, level)];
        if (!e->present) {
            e->next = new_table();
            if (e->next == NULL) return false;
            e->present = true;
        }
        t = e->next;
    }
    Entry *pte = &t->e[index_of(va, 0)];
    pte->present = true;
    pte->frame = frame;
    return true;
}

/* The same walk as translate(), printing every read. */
static void trace(const Table *pml4, uint64_t va) {
    static const char *names[] = {"PT  ", "PD  ", "PDPT", "PML4"};
    const Table *t = pml4;
    printf("walk 0x%" PRIx64 "\n", va);
    for (int level = 3; level >= 0; --level) {
        const Entry *e = &t->e[index_of(va, level)];
        printf("  read %d: %s[%" PRIu64 "] %s\n", 4 - level, names[level], index_of(va, level),
               e->present ? (level ? "-> next table" : "-> frame") : "not present: PAGE FAULT");
        if (!e->present) return;
        if (level) t = e->next;
        else printf("  physical 0x%" PRIx64 "\n", (e->frame << 12) | (va & 0xFFF));
    }
}

int main(void) {
    Table *pml4 = new_table();
    if (pml4 == NULL
        || !map_page(pml4, 0x00007f3a5c2d1000ULL, 0x1a2b3)      /* the page of the address of the slide */
        || !map_page(pml4, 0x00007f3a5c2d2000ULL, 0x0c411)      /* its neighbour: same PT, next entry */
        || !map_page(pml4, 0x0000000000400000ULL, 0x7e090)) {   /* far away: needs its own PDPT, PD and PT */
        printf("out of memory building the tables\n");
        free_tables();
        return 1;
    }
    printf("3 pages mapped with %zu tables of 512 entries: the tree only grows where something is mapped\n"
           "(one flat table for 48 bits would need 2 to the 36 entries)\n\n", g_table_count);

    trace(pml4, 0x00007f3a5c2d1e48ULL);
    trace(pml4, 0x00007f3a5c2d1008ULL);              /* same page: the same four reads, only the offset differs */
    trace(pml4, 0x00007f3a5c2d2010ULL);
    trace(pml4, 0x00007f3a5c2d3000ULL);              /* not mapped */

    uint64_t a = 0, b = 0, c = 0;
    bool ok = translate(pml4, 0x00007f3a5c2d1e48ULL, &a) && a == 0x1a2b3e48ULL
           && translate(pml4, 0x00007f3a5c2d2010ULL, &b) && b == 0x0c411010ULL
           && translate(pml4, 0x0000000000400abcULL, &c) && c == 0x7e090abcULL;
    bool faulted = false;
    uint64_t pa = 0;
    const uint64_t unmapped = 0x00007f3a5c2d3000ULL;
    if (translate(pml4, unmapped, &pa)) {
        printf("unexpected translation 0x%" PRIx64 "\n", pa);
    } else {
        faulted = true;
        printf("\ntranslate returned a page fault for 0x%" PRIx64 ": the real CPU calls the kernel here\n",
               unmapped);
    }
    printf("translations %s, unmapped address %s\n", ok ? "correct" : "WRONG",
           faulted ? "faulted" : "DID NOT FAULT");
    free_tables();
    return ok && faulted ? 0 : 1;
}
