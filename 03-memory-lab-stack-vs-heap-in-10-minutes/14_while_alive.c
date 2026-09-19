/* Stack vs Heap: Where Does Your Data Actually Live? - slide 14: thank you (C version of 14_while_alive.cpp) */
/* Build: make 14_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdio.h>
#include <stdlib.h>

/* the imaginary tomjnet.h */
typedef struct { char name[64]; } Topic;
static int episodes = 0;
static int alive(void) { return episodes < 3; }               /* three episodes, then the demo ends */
static Topic next_memory_topic(void) {
    static const char *topics[] = { "Virtual Memory in 10 Minutes", "Pages and Page Tables",
                                    "CPU Cache Explained: L1, L2, L3 and Cache Lines" };
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

/* C has no std::vector and no unique_ptr: a growable array with one owner, freed by hand */
typedef struct { Topic *items; size_t len; size_t cap; } TopicList;

static int push_back(TopicList *l, Topic t) {
    if (l->len == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 4;
        Topic *grown = realloc(l->items, cap * sizeof *grown);
        if (grown == NULL) return 0;
        l->items = grown;
        l->cap = cap;
    }
    l->items[l->len++] = t;
    return 1;
}

int main(void) {
    TopicList lab = { NULL, 0, 0 };           /* one owner, on the stack */
    int ok = 1;
    while (alive()) {
        ok = ok && push_back(&lab, next_memory_topic());  /* next: virtual memory */
        ok = ok && push_back(&lab, viewer_request());     /* on the heap, owned */
        subscribe();                                      /* lifetime benefit */
    }
    if (ok) {
        printf("the Memory Lab queue, owned by one TopicList on the stack:\n");
        for (size_t i = 0; i < lab.len; ++i) printf("  %s\n", lab.items[i].name);
    }
    free(lab.items);                          /* the delete unique_ptr would run here */
    return ok ? 0 : 1;
}
