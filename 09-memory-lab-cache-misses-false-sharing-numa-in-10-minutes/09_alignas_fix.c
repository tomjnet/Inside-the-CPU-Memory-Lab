/* Memory Performance: Cache Misses, False Sharing and NUMA - slide 9: the fix: alignas(64) (C version of 09_alignas_fix.cpp) */
/* Build: make 09_alignas_fix_c */
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

typedef struct {                   /* slide 8: 16 bytes, both counters in one cache line */
    atomic_ullong a;
    atomic_ullong b;
} Counters;
typedef struct { _Alignas(64) Counters c; } OneLine;   /* as in 08_false_sharing: c starts on a line boundary */

typedef struct {                   /* 128 bytes: one cache line each */
    _Alignas(64) atomic_ullong a;
    _Alignas(64) atomic_ullong b;
} Padded;
_Static_assert(sizeof(Padded) == 128 && _Alignof(Padded) == 64, "one cache line per counter");

static int work(void *arg) {
    atomic_ullong *n = arg;
    for (int i = 0; i < kIters; ++i) atomic_fetch_add(n, 1);  /* a write */
    return 0;
}

static int g_thread_failed = 0;

/* C has no templates and no references: the two counters arrive as pointers */
static void run_two_threads(atomic_ullong *x, atomic_ullong *y) {
    thrd_t t1, t2;
    int ok1 = thrd_create(&t1, work, (void *)x) == thrd_success;
    int ok2 = thrd_create(&t2, work, (void *)y) == thrd_success;
    if (ok1) thrd_join(t1, NULL);
    if (ok2) thrd_join(t2, NULL);
    if (!ok1 || !ok2) g_thread_failed = 1;
}

static void shared_run(void *ctx) { Counters *c = ctx; run_two_threads(&c->a, &c->b); }
static void padded_run(void *ctx) { Padded *p = ctx; run_two_threads(&p->a, &p->b); }

int main(void) {
    static OneLine line;           /* static: the 64 byte alignment is kept on every compiler */
    Counters *c = &line.c;
    atomic_init(&c->a, 0);
    atomic_init(&c->b, 0);
    BenchResult shared = bench_ns(shared_run, c, 3, 21);

    static Padded p;
    atomic_init(&p.a, 0);
    atomic_init(&p.b, 0);
    BenchResult padded = bench_ns(padded_run, &p, 3, 21);
    if (g_thread_failed) {
        printf("FAIL: could not start a thread\n");
        return 1;
    }
    /* same work, no bouncing line: often several times faster */
    double ratio = shared.median_ns / padded.median_ns;

    printf("sizeof(Counters) = %zu bytes, sizeof(Padded) = %zu bytes, alignof(Padded) = %zu\n",
           sizeof(Counters), sizeof(Padded), (size_t)_Alignof(Padded));
    printf("two threads, %d fetch_add each, median of 21 runs (this machine):\n", kIters);
    printf("  one shared line     : %.2f ms\n", shared.median_ns / 1e6);
    printf("  one line per counter: %.2f ms\n", padded.median_ns / 1e6);
    printf("  ratio               : %.2fx (a virtual machine that shares cores can hide it)\n", ratio);

    /* correctness: both layouts count every update of 3 warm up runs plus 21 timed runs */
    const unsigned long long expected = 24ull * (unsigned long long)kIters;
    if (atomic_load(&c->a) != expected || atomic_load(&c->b) != expected ||
        atomic_load(&p.a) != expected || atomic_load(&p.b) != expected) {
        printf("FAIL: a counter lost updates\n");
        return 1;
    }
    printf("all four counters = %llu: same result, different layout\n", expected);
    return 0;
}
