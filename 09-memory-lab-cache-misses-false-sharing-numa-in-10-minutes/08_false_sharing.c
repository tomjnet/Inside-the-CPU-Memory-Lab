/* Memory Performance: Cache Misses, False Sharing and NUMA - slide 8: false sharing, measured (C version of 08_false_sharing.cpp) */
/* Build: make 08_false_sharing_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#pragma warning(disable : 4324)      /* "structure was padded due to alignment specifier": that is the point of _Alignas(64) */
#endif

#include <stdatomic.h>
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

/* ---- timing harness: the C twin of bench_ns ---- */
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

enum { kIters = 1000000 };         /* writes per thread and per run */

typedef struct {                   /* 16 bytes: both in ONE cache line */
    atomic_ullong a;
    atomic_ullong b;
} Counters;

/* The slide writes "Counters c;". A 16 byte object with 8 byte alignment can straddle two cache lines one time out
   of eight; this wrapper starts c on a line boundary, so a and b always share one line and every run shows the effect. */
typedef struct { _Alignas(64) Counters c; } OneLine;

static int work(void *arg) {
    atomic_ullong *n = arg;
    for (int i = 0; i < kIters; ++i) atomic_fetch_add(n, 1);  /* a write */
    return 0;
}

static int g_thread_failed = 0;

/* no data is shared, the line is */
static void shared_run(void *ctx) {
    Counters *c = ctx;
    thrd_t t1, t2;
    int ok1 = thrd_create(&t1, work, (void *)&c->a) == thrd_success;   /* core 1 owns the line */
    int ok2 = thrd_create(&t2, work, (void *)&c->b) == thrd_success;   /* core 2 takes it back */
    if (ok1) thrd_join(t1, NULL);
    if (ok2) thrd_join(t2, NULL);
    if (!ok1 || !ok2) g_thread_failed = 1;
}

int main(void) {
    static OneLine line;           /* static: the 64 byte alignment is kept on every compiler */
    Counters *c = &line.c;
    atomic_init(&c->a, 0);
    atomic_init(&c->b, 0);
    BenchResult shared = bench_ns(shared_run, c, 3, 21);
    if (g_thread_failed) {
        printf("FAIL: could not start a thread\n");
        return 1;
    }

    const uintptr_t line_a = (uintptr_t)&c->a / 64;
    const uintptr_t line_b = (uintptr_t)&c->b / 64;
    printf("sizeof(Counters) = %zu bytes, a and b in the same 64 byte line: %s\n", sizeof(Counters),
           line_a == line_b ? "yes" : "no");
    printf("two threads, %d fetch_add each, one counter per thread (this machine):\n", kIters);
    printf("  median %.2f ms per run, %.2f ns per write\n", shared.median_ns / 1e6, shared.median_ns / kIters);
    printf("09_alignas_fix runs the same work with one cache line per counter and prints the ratio\n");

    /* correctness: 3 warm up runs plus 21 timed runs, nothing lost */
    const unsigned long long expected = 24ull * (unsigned long long)kIters;
    const unsigned long long a = atomic_load(&c->a), b = atomic_load(&c->b);
    if (a != expected || b != expected) {
        printf("FAIL: a counter lost updates\n");
        return 1;
    }
    printf("a = %llu, b = %llu: every update counted\n", a, b);
    return 0;
}
