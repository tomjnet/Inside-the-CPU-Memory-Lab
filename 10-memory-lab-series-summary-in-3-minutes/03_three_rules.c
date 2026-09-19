/* Inside the CPU: What We Learned About Memory - slide 3: three rules: small, contiguous, local (C version of 03_three_rules.cpp) */
/* Build: make 03_three_rules_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#pragma warning(disable : 4324)      /* "structure was padded due to alignment specifier": that is the point of _Alignas(64) */
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- threads: C11 <threads.h>; MinGW lacks it, so map the two calls onto pthreads ---- */
#if defined(__MINGW32__)
#include <pthread.h>
typedef pthread_t thrd_t;
typedef int (*thrd_start_t)(void *);
enum { thrd_success = 0, thrd_error = 2 };
struct thrd_boot { thrd_start_t fn; void *arg; };
static void *thrd_boot_run(void *p) {
    struct thrd_boot b = *(struct thrd_boot *)p;
    free(p);
    return (void *)(intptr_t)b.fn(b.arg);
}
static int thrd_create(thrd_t *t, thrd_start_t fn, void *arg) {
    struct thrd_boot *b = malloc(sizeof *b);
    if (b == NULL) return thrd_error;
    b->fn = fn;
    b->arg = arg;
    if (pthread_create(t, NULL, thrd_boot_run, b) != 0) { free(b); return thrd_error; }
    return thrd_success;
}
static int thrd_join(thrd_t t, int *res) {
    void *r = NULL;
    if (pthread_join(t, &r) != 0) return thrd_error;
    if (res != NULL) *res = (int)(intptr_t)r;
    return thrd_success;
}
#else
#include <threads.h>
#endif

/* ---- timing harness: the C twin of bench_ns and sink ---- */
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double now_ns(void) {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1e9 / (double)f.QuadPart;
}
#else
#include <time.h>
static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

static volatile uint64_t g_sink = 0;
static void sink(uint64_t x) { g_sink = g_sink + x; }   /* keeps the result alive */

typedef struct { double min_ns; double median_ns; } BenchResult;

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* warm up, repeat, keep the minimum and the median; at most 64 repeats */
static BenchResult bench_ns(void (*f)(void *), void *ctx, int warmup, int repeats) {
    double samples[64];
    if (repeats > 64) repeats = 64;
    for (int i = 0; i < warmup; ++i) f(ctx);
    for (int i = 0; i < repeats; ++i) {
        double t0 = now_ns();
        f(ctx);
        samples[i] = now_ns() - t0;
    }
    qsort(samples, (size_t)repeats, sizeof samples[0], cmp_double);
    BenchResult r = { samples[0], samples[repeats / 2] };
    return r;
}

typedef struct { char tag; double price; char side; } Loose;   /* 24 bytes */
typedef struct { double price; char tag; char side; } Tight;   /* small: 16 */
typedef struct { _Alignas(64) long n; } Counter;               /* local: one line per thread */

/* fixed-seed splitmix64: every run does the same work (the order differs from the C++ mt19937 run) */
static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* C has no lambda: the captured book and order travel in a context struct */
typedef struct { const Tight *book; const size_t *order; size_t n; } SumCtx;

static void sum_in_order(void *p) {       /* in order: prefetcher helps */
    const SumCtx *c = p;
    double s = 0;
    for (size_t i = 0; i < c->n; ++i) s += c->book[i].price;
    sink((uint64_t)s);
}
static void sum_shuffled(void *p) {       /* shuffled: a miss per step */
    const SumCtx *c = p;
    double s = 0;
    for (size_t i = 0; i < c->n; ++i) s += c->book[c->order[i]].price;
    sink((uint64_t)s);
}

static Counter counters[2];
static const long rounds = 5000000;

static int bump_a(void *arg) { (void)arg; for (long k = 0; k < rounds; ++k) counters[0].n = counters[0].n + 1; return 0; }
static int bump_b(void *arg) { (void)arg; for (long k = 0; k < rounds; ++k) counters[1].n = counters[1].n + 2; return 0; }

int main(void) {
    const size_t n = (size_t)1 << 20;
    Tight *book = malloc(n * sizeof *book);        /* contiguous: one block */
    size_t *order = malloc(n * sizeof *order);
    int rc = 1;
    if (book == NULL || order == NULL) { printf("FAIL: out of memory\n"); goto done; }

    /* the data and the shuffled index the slide assumes: whole numbers, so both sums are exact in a double */
    for (size_t i = 0; i < n; ++i) { book[i].price = (double)(i % 100); book[i].tag = 'B'; book[i].side = 'S'; }
    for (size_t i = 0; i < n; ++i) order[i] = i;
    uint64_t rng = 12345;                          /* fixed seed: every run does the same work */
    for (size_t i = n - 1; i > 0; --i) {           /* Fisher Yates */
        size_t j = (size_t)(splitmix64(&rng) % (uint64_t)(i + 1));
        size_t t = order[i]; order[i] = order[j]; order[j] = t;
    }

    SumCtx ctx = { book, order, n };
    BenchResult seq = bench_ns(sum_in_order, &ctx, 3, 21);
    BenchResult rnd = bench_ns(sum_shuffled, &ctx, 3, 21);

    /* rule 1, small: same members, two orders */
    printf("rule 1, keep data small\n");
    printf("  sizeof(Loose) = %zu bytes, alignof %zu\n", sizeof(Loose), (size_t)alignof(Loose));
    printf("  sizeof(Tight) = %zu bytes, alignof %zu\n", sizeof(Tight), (size_t)alignof(Tight));
    printf("  per 64 byte cache line: %zu Loose or %zu Tight\n", 64 / sizeof(Loose), 64 / sizeof(Tight));
    if (sizeof(Tight) > sizeof(Loose)) { printf("FAIL: reordering made the struct bigger\n"); goto done; }

    /* rule 2, contiguous: same elements, same sum, two orders of access */
    double in_order = 0, shuffled = 0;
    for (size_t i = 0; i < n; ++i) in_order += book[i].price;
    for (size_t i = 0; i < n; ++i) shuffled += book[order[i]].price;
    printf("rule 2, keep it contiguous (%zu elements, %zu MiB)\n", n, n * sizeof(Tight) / (1024 * 1024));
    printf("  in order: min %g ms, median %g ms\n", seq.min_ns / 1e6, seq.median_ns / 1e6);
    printf("  shuffled: min %g ms, median %g ms\n", rnd.min_ns / 1e6, rnd.median_ns / 1e6);
    printf("  shuffled / in order, this machine: %gx (median)\n", rnd.median_ns / seq.median_ns);
    if (in_order != shuffled) { printf("FAIL: the two sums differ\n"); goto done; }
    printf("  same sum both ways: %lld\n", (long long)in_order);

    /* rule 3, local: two threads, each one writes only its own cache line */
    thrd_t a, b;
    if (thrd_create(&a, bump_a, NULL) != thrd_success) { printf("FAIL: thread start\n"); goto done; }
    if (thrd_create(&b, bump_b, NULL) != thrd_success) { thrd_join(a, NULL); printf("FAIL: thread start\n"); goto done; }
    thrd_join(a, NULL);
    thrd_join(b, NULL);
    uintptr_t a0 = (uintptr_t)&counters[0];
    uintptr_t a1 = (uintptr_t)&counters[1];
    printf("rule 3, keep it local\n");
    printf("  sizeof(Counter) = %zu, alignof %zu\n", sizeof(Counter), (size_t)alignof(Counter));
    printf("  the two counters are %llu bytes apart: one cache line each, no false sharing\n",
           (unsigned long long)(a1 - a0));
    printf("  counters: %ld and %ld\n", counters[0].n, counters[1].n);
    if (a0 % 64 != 0 || a1 % 64 != 0 || a1 - a0 < 64) { printf("FAIL: a counter is not on its own cache line\n"); goto done; }
    if (counters[0].n != rounds || counters[1].n != 2 * rounds) { printf("FAIL: a count was lost\n"); goto done; }
    rc = 0;

done:                                              /* C has no destructor: one cleanup label frees both blocks */
    free(order);
    free(book);
    return rc;
}
