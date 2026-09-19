/* Memory Performance: Cache Misses, False Sharing and NUMA - slide 13: thank you (C version of 13_while_alive.cpp) */
/* Build: make 13_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdio.h>
#include <stdlib.h>

/* the imaginary tomjnet.h of the slide */
typedef struct { char name[80]; } Topic;
static int episodes = 0;
static int alive(void) { return episodes < 3; }               /* three episodes, then the demo ends */
static Topic next_memory_topic(void) {
    static const char *topics[] = { "Inside the CPU: What We Learned About Memory (episode 10)",
                                    "the Low Latency C++ Lab", "the Cache and NUMA Lab" };
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

/* C has no std::vector: a growable array of contiguous Topics, freed by hand */
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
    TopicList memory_lab = { NULL, 0, 0 };
    int ok = 1;
    while (alive()) {
        ok = ok && push_back(&memory_lab, next_memory_topic());  /* contiguous */
        ok = ok && push_back(&memory_lab, viewer_request());     /* same cache line */
        subscribe();                                             /* lifetime benefit */
    }

    if (ok) {
        printf("next in the Memory Lab and after it:\n");
        for (size_t i = 0; i < memory_lab.len; ++i) printf("  %s\n", memory_lab.items[i].name);
    }
    free(memory_lab.items);
    return ok ? 0 : 1;
}
