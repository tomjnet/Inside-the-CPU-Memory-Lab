/* Inside the CPU: What We Learned About Memory - slide 5: thank you (C version of 05_while_alive.cpp) */
/* Build: make 05_while_alive_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* the imaginary tomjnet.h of the slide */
/* C has no std::string: the name lives inside the struct */
typedef struct { char name[64]; } Lab;
static int episodes = 0;
static int alive(void) { return episodes < 2; }                 /* two labs, then the demo ends */
static Lab next_lab(void) {
    static const char *labs[] = {"Inside the CPU: Low Latency C++ Lab", "Inside the CPU: Cache and NUMA Lab"};
    Lab l;
    snprintf(l.name, sizeof l.name, "%s", labs[episodes++]);
    return l;
}
static void measure(const Lab *lab) { printf("next lab: %s\n", lab->name); }
static void subscribe(void) { printf("  subscribed to TomJNet\n"); }

/* C has no std::vector: a pointer, a size and a capacity; push grows with realloc only when full */
typedef struct { Lab *data; size_t size; size_t capacity; } LabList;

static int push(LabList *v, Lab lab) {
    if (v->size == v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 1;
        Lab *p = realloc(v->data, cap * sizeof *p);
        if (p == NULL) return 0;
        v->data = p;
        v->capacity = cap;
    }
    v->data[v->size++] = lab;
    return 1;
}

int main(void) {
    LabList next_labs = {NULL, 0, 0};
    next_labs.data = malloc(2 * sizeof *next_labs.data);   /* the reserve(2): small, contiguous, local */
    if (next_labs.data == NULL) { printf("FAIL: out of memory\n"); return 1; }
    next_labs.capacity = 2;
    const Lab *before = next_labs.data;
    while (alive()) {
        if (!push(&next_labs, next_lab())) {                /* Cache and NUMA, Low Latency */
            printf("FAIL: out of memory\n");
            free(next_labs.data);
            return 1;
        }
        measure(&next_labs.data[next_labs.size - 1]);      /* your machine, your numbers */
        subscribe();                                        /* lifetime benefit */
    }
    int same = next_labs.data == before;
    printf("%zu labs in one contiguous block, reallocated: %s (reserve did its job)\n",
           next_labs.size, same ? "no" : "yes");
    free(next_labs.data);                                   /* C has no destructor: free by hand */
    return same ? 0 : 1;
}
