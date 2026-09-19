/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 5: inside a page table entry (C version of 05_page_table_entry.cpp) */
/* Build: make 05_page_table_entry_c */
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

/* model of a page table entry: 8 bytes that map one 4 KiB page.
   ISO C allows bit-fields only on int, unsigned int and _Bool, so a 40 bit frame field is not portable:
   the C way is one uint64_t and shifts and masks, which is also how the kernel itself reads an entry. */
typedef struct { uint64_t bits; } Entry;

enum {
    PRESENT  = 0,   /* 0: the access is a page fault */
    WRITABLE = 1,   /* 0: a write faults (copy on write) */
    USER     = 2,   /* 0: kernel only */
    ACCESSED = 3,   /* set by the CPU on any access */
    DIRTY    = 4,   /* set by the CPU on a write */
    FRAME    = 5,   /* bits 5 to 44: physical frame number */
    NO_EXEC  = 45   /* 1: fetching code here faults */
};
#define FRAME_MASK ((1ULL << 40) - 1)
/* 512 entries x 8 bytes = 4096 bytes: a table is itself one page */

/* A model: the real x86 64 entry keeps present, writable and user in bits 0 to 2, accessed and dirty in bits
   5 and 6, the frame number from bit 12 up and no execute in bit 63. The fields are the same. */
_Static_assert(sizeof(Entry) == 8, "one entry is 8 bytes");
_Static_assert(sizeof(Entry) * 512 == 4096, "a table of 512 entries is one page");

static unsigned get(const Entry *e, int bit) { return (unsigned)((e->bits >> bit) & 1u); }
static void set(Entry *e, int bit) { e->bits |= 1ULL << bit; }
static uint64_t frame(const Entry *e) { return (e->bits >> FRAME) & FRAME_MASK; }
static void set_frame(Entry *e, uint64_t f) {
    e->bits = (e->bits & ~(FRAME_MASK << FRAME)) | ((f & FRAME_MASK) << FRAME);
}

typedef enum { ACCESS_READ, ACCESS_WRITE, ACCESS_EXECUTE } Access;

static const char *name(Access a) {
    return a == ACCESS_READ ? "read   " : a == ACCESS_WRITE ? "write  " : "execute";
}

/* What the CPU does with one entry on one access: check the flags, then set accessed and dirty. */
static bool access_page(Entry *e, Access a, bool user_mode) {
    const char *why = NULL;
    if (!get(e, PRESENT)) why = "not present";
    else if (user_mode && !get(e, USER)) why = "kernel only page";
    else if (a == ACCESS_WRITE && !get(e, WRITABLE)) why = "read only page";
    else if (a == ACCESS_EXECUTE && get(e, NO_EXEC)) why = "no execute page";
    printf("  %s%s%s\n", name(a), why ? " -> page fault: " : " -> ok", why ? why : "");
    if (why) return false;
    set(e, ACCESSED);
    if (a == ACCESS_WRITE) set(e, DIRTY);
    return true;
}

static void show(const char *label, const Entry *e) {
    printf("%s: present=%u writable=%u user=%u accessed=%u dirty=%u no_exec=%u frame=0x%" PRIx64 "\n",
           label, get(e, PRESENT), get(e, WRITABLE), get(e, USER), get(e, ACCESSED), get(e, DIRTY),
           get(e, NO_EXEC), frame(e));
}

int main(void) {
    printf("sizeof(Entry) = %zu bytes, 512 entries = %zu bytes: a table is itself one page\n\n",
           sizeof(Entry), sizeof(Entry) * 512);

    Entry data = {0};                   /* a page of a writable variable */
    set(&data, PRESENT); set(&data, WRITABLE); set(&data, USER); set(&data, NO_EXEC); set_frame(&data, 0x1a2b3);
    show("data page  ", &data);
    int faults = 0;
    if (!access_page(&data, ACCESS_READ, true)) ++faults;
    if (!access_page(&data, ACCESS_WRITE, true)) ++faults;
    if (!access_page(&data, ACCESS_EXECUTE, true)) ++faults;  /* the only fault of the three */
    show("after      ", &data);

    Entry text = {0};                   /* a page of code: read and execute, never write */
    set(&text, PRESENT); set(&text, USER); set_frame(&text, 0x0c411);
    printf("\n");
    show("code page  ", &text);
    if (!access_page(&text, ACCESS_EXECUTE, true)) ++faults;
    if (!access_page(&text, ACCESS_WRITE, true)) ++faults;    /* fault: read only */

    Entry fresh = {0};                  /* mapped by mmap, never touched: the first access is a minor fault */
    printf("\n");
    show("fresh page ", &fresh);
    if (!access_page(&fresh, ACCESS_READ, true)) ++faults;    /* fault: not present */

    printf("\n%d page faults out of 6 accesses\n", faults);
    printf("protection is per page: the flags live in the entry, and an entry maps 4096 bytes at once\n");
    return (faults == 3 && get(&data, ACCESSED) && get(&data, DIRTY) && !get(&text, DIRTY)) ? 0 : 1;
}
