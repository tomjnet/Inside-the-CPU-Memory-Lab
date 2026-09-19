/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 13: thank you (C version of 13_while_alive.cpp) */
/* Build: make 13_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it) */
typedef struct { char name[96]; } Topic;        /* no std::string in C: a fixed char buffer */
static int episodes = 0;
static bool alive(void) { return episodes < 3; }             /* three episodes, then the demo ends */
static Topic next_memory_topic(void) {
    static const char *topics[] = {"CPU Cache Explained: L1, L2, L3 and Cache Lines",
                                   "Memory Alignment and Locality: Why Data Layout Matters",
                                   "Build a Simple Memory Allocator: Understanding malloc and new"};
    Topic t;
    snprintf(t.name, sizeof t.name, "%s", topics[episodes++]);
    return t;
}
static Topic viewer_request(void) {
    Topic t;
    snprintf(t.name, sizeof t.name, "viewer request #%d", episodes);
    return t;
}
static void subscribe(void) { printf("  subscribed to TomJNet\n"); }

int main(void) {
    Topic memory_lab[8];                        /* no std::vector in C: a fixed array plus a count */
    size_t count = 0;
    while (alive() && count + 2 <= sizeof memory_lab / sizeof memory_lab[0]) {
        memory_lab[count++] = next_memory_topic(); /* CPU cache next */
        memory_lab[count++] = viewer_request();    /* same page */
        subscribe();                               /* lifetime benefit */
    }

    printf("the Memory Lab queue, %zu topics:\n", count);
    for (size_t i = 0; i < count; ++i) printf("  %s\n", memory_lab[i].name);
    return 0;
}
