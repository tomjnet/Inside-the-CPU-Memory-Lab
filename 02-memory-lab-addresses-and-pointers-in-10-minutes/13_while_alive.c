/* Memory Addresses and Pointers: What Is Really Stored? - slide 13: thank you (C version of 13_while_alive.cpp) */
/* Build: make 13_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it) */
typedef struct { const char *name; } Topic;
static int episodes = 0;
static int alive(void) { return episodes < 3; }             /* three episodes, then the demo ends */
static Topic next_memory_topic(void) {
    static const char *const topics[] = {"Stack vs Heap: Where Does Your Data Actually Live?",
                                         "Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory",
                                         "Pages and Page Tables: How Virtual Addresses Reach RAM"};
    Topic t = {topics[episodes++]};
    return t;
}
static void watch(const Topic *t) {
    printf("  watching %p: %s\n", (const void *)t, t->name);
}
static void subscribe(void) { printf("  subscribed to TomJNet\n"); }

int main(void) {
    /* C has no std::vector: a heap array, its length and its capacity, grown with realloc */
    Topic *memory_lab = NULL;
    size_t len = 0, cap = 0;
    while (alive()) {
        if (len == cap) {
            const size_t new_cap = cap ? cap * 2 : 1;
            Topic *grown = realloc(memory_lab, new_cap * sizeof *grown);
            if (grown == NULL) { free(memory_lab); return 1; }
            memory_lab = grown;
            cap = new_cap;
        }
        memory_lab[len++] = next_memory_topic();   /* stack vs heap */
        Topic *latest = &memory_lab[len - 1];      /* address + type */
        watch(latest);                             /* never NULL */
        subscribe();                               /* lifetime benefit */
    }

    printf("the Memory Lab queue, %zu topics:\n", len);
    for (size_t i = 0; i < len; ++i) printf("  %s\n", memory_lab[i].name);
    free(memory_lab);
    return 0;
}
