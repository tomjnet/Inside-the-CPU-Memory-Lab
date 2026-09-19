/* Memory Alignment and Locality: Why Data Layout Matters - slide 13: thank you (C version of 13_while_alive.cpp) */
/* Build: make 13_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it) */
typedef struct { char name[80]; } Topic;       /* the text lives inside the struct: no std::string */
static int episodes = 0;
static int alive(void) { return episodes < 3; }             /* three episodes, then the demo ends */
static Topic next_memory_topic(void) {
    static const char *topics[] = {"Build a Simple Memory Allocator: Understanding malloc and new",
                                   "Memory Performance: Cache Misses, False Sharing and NUMA",
                                   "Inside the CPU: What We Learned About Memory"};
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

/* C has no std::vector: a pointer, a length and a capacity, grown with realloc, freed by hand */
typedef struct { Topic *data; size_t size; size_t cap; } TopicList;
static int push_back(TopicList *v, Topic t) {
    if (v->size == v->cap) {
        size_t cap = v->cap ? v->cap * 2 : 2;
        Topic *p = realloc(v->data, cap * sizeof *p);
        if (p == NULL) return 1;
        v->data = p;
        v->cap = cap;
    }
    v->data[v->size++] = t;
    return 0;
}

int main(void) {
    TopicList memory_lab = {NULL, 0, 0};     /* contiguous, no padding */
    int rc = 0;
    while (alive()) {
        if (push_back(&memory_lab, next_memory_topic()) != 0) { rc = 1; goto done; } /* next: allocator */
        if (push_back(&memory_lab, viewer_request()) != 0) { rc = 1; goto done; }    /* same cache line */
        subscribe();                                                                 /* lifetime benefit */
    }

    printf("the Memory Lab queue, %zu topics in one contiguous block of %zu bytes (sizeof(Topic) = %zu, alignof %zu):\n",
           memory_lab.size, memory_lab.size * sizeof(Topic), sizeof(Topic), alignof(Topic));
    for (size_t i = 0; i < memory_lab.size; ++i) printf("  %s\n", memory_lab.data[i].name);
done:
    free(memory_lab.data);          /* no destructor in C: the free is ours */
    return rc;
}
