/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 14: thank you (C version of 14_while_alive.cpp) */
/* Build: make 14_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdio.h>

/* The imaginary "tomjnet.h" of the slide, written out so the program builds. */
typedef struct { const char *name; } Topic;
typedef struct { const char *title; int resident; } Page;

#define EPISODES 3                                           /* three episodes, then the demo ends */

static size_t touched = 0;
static int alive(void) { return touched < EPISODES; }

/* A fixed array instead of std::vector: the three pages are known up front. */
typedef struct {
    Page pages[EPISODES];                                    /* reserved: titles only, nothing resident */
    size_t count;
} AddressSpace;

/* C has no references: touch hands back a pointer into the space. */
static Page *touch(AddressSpace *space) {                    /* first touch: the page becomes real */
    Page *page = &space->pages[touched++];
    page->resident = 1;
    printf("  page fault: \"%s\" is now resident\n", page->title);
    return page;
}

static Topic next_episode(void) {
    Topic t = { "Pages and Page Tables: How Virtual Addresses Reach RAM" };
    return t;
}

static AddressSpace reserve(const Topic *next) {
    AddressSpace space = { { { next->name, 0 },
                             { "CPU Cache Explained: L1, L2, L3 and Cache Lines", 0 },
                             { "Memory Alignment and Locality: Why Data Layout Matters", 0 } },
                           EPISODES };
    printf("reserved %zu episodes of the Memory Lab, none resident yet\n", space.count);
    return space;
}

static void learn(const Page *page) { printf("  learned: %s\n", page->title); }
static void subscribe(void) { printf("  subscribed to TomJNet\n"); }

int main(void) {
    Topic next = next_episode();
    AddressSpace mine = reserve(&next);  /* no RAM yet */
    while (alive()) {
        Page *page = touch(&mine);       /* page fault: now it is real */
        learn(page);                     /* 4 KiB at a time */
        subscribe();                     /* lifetime mapping */
    }
    return 0;
}
