/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 13: thank you (C version of 13_while_alive.cpp) */
/* Build: make 13_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stddef.h>
#include <stdio.h>

/* #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it) */
/* C has no std::string: the name lives inside the struct, so one Topic is exactly one 64 byte line */
typedef struct { char name[64]; } Topic;
static int episodes = 0;
static int alive(void) { return episodes < 3; }             /* three episodes, then the demo ends */
static Topic next_memory_topic(void) {
    static const char *topics[] = {"Memory Alignment and Locality: Why Data Layout Matters",
                                   "Build a Simple Memory Allocator: Understanding malloc and new",
                                   "Memory Performance: Cache Misses, False Sharing and NUMA"};
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
    /* C has no std::vector: a fixed array plus a count, still one contiguous block */
    Topic memory_lab[8];
    size_t count = 0;
    while (alive() && count + 2 <= sizeof memory_lab / sizeof memory_lab[0]) {
        memory_lab[count++] = next_memory_topic();   /* same line: hit */
        memory_lab[count++] = viewer_request();      /* prefetched */
        subscribe();                                 /* lifetime benefit */
    }

    printf("the Memory Lab queue, %zu topics in one contiguous block:\n", count);
    for (size_t i = 0; i < count; ++i) printf("  %s\n", memory_lab[i].name);
    printf("sizeof(Topic) = %zu bytes: %s in one 64 byte cache line\n", sizeof(Topic),
           sizeof(Topic) <= 64 ? "fits" : "does not fit");
    return 0;
}
