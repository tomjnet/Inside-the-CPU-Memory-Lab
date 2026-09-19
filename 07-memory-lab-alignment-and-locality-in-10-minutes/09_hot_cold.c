/* Memory Alignment and Locality: Why Data Layout Matters - slide 9: hot and cold splitting (C version of 09_hot_cold.cpp) */
/* Build: make 09_hot_cold_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* fixed seed xorshift64 in place of std::mt19937: every run does the same work */
static uint64_t rng_state = 12345;
static uint64_t xorshift64(void) {
    uint64_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return rng_state = x;
}

typedef struct {                  /* 96 bytes: the scan drags it all */
    uint64_t id; double price; uint32_t qty;   /* hot */
    char client[64]; uint64_t created;         /* cold */
} OrderFat;
typedef struct {                  /* 24 bytes: what the loop reads */
    uint64_t id; double price; uint32_t qty;
} OrderHot;
typedef struct {                  /* 72 bytes: read once per report */
    char client[64]; uint64_t created;
} OrderCold;

/* C has no lambda: the context struct carries what the C++ lambda captured */
typedef struct { const OrderFat *fat; const OrderHot *hot; size_t n; double total; } ScanCtx;

static void scan_fat(void *ctx) {
    ScanCtx *c = ctx;
    double s = 0;
    for (size_t i = 0; i < c->n; ++i) s += c->fat[i].price * c->fat[i].qty;
    c->total = s;
    sink((uint64_t)s);
}
static void scan_hot(void *ctx) {
    ScanCtx *c = ctx;
    double s = 0;
    for (size_t i = 0; i < c->n; ++i) s += c->hot[i].price * c->hot[i].qty;
    c->total = s;
    sink((uint64_t)s);
}

int main(void) {
    const size_t n = 500000;
    int rc = 1;
    OrderHot  *hot  = calloc(n, sizeof *hot);      /* hot[i] and cold[i]: same order */
    OrderCold *cold = calloc(n, sizeof *cold);     /* the index is the link */
    OrderFat  *fat  = calloc(n, sizeof *fat);
    if (hot == NULL || cold == NULL || fat == NULL) {
        printf("FAIL: out of memory\n");
        goto done;
    }
    for (size_t i = 0; i < n; ++i) {               /* whole numbers, so both totals are exact and must be equal */
        OrderFat o;
        memset(&o, 0, sizeof o);                   /* zeroed: the client text stays terminated */
        o.id = i;
        o.price = (double)(1 + xorshift64() % 500);
        o.qty = (uint32_t)(1 + xorshift64() % 100);
        snprintf(o.client, sizeof o.client, "client-%zu", i % 1000);
        o.created = 1700000000u + i;
        fat[i] = o;
        hot[i].id = o.id;
        hot[i].price = o.price;
        hot[i].qty = o.qty;
        memcpy(cold[i].client, o.client, sizeof o.client);
        cold[i].created = o.created;
    }

    /* The hot loop: the notional of the whole book, price times quantity. It never reads client or created. */
    ScanCtx cf = { fat, hot, n, 0 }, ch = { fat, hot, n, 0 };
    BenchResult t_fat = bench_ns(scan_fat, &cf, 3, 21);
    BenchResult t_hot = bench_ns(scan_hot, &ch, 3, 21);
    const double total_fat = cf.total, total_hot = ch.total;
    const int equal = (uint64_t)total_fat == (uint64_t)total_hot;

    /* The cold path: one report line, found through the shared index. */
    size_t best = 0;
    for (size_t i = 1; i < n; ++i)
        if (hot[i].price * hot[i].qty > hot[best].price * hot[best].qty) best = i;

    printf("sizeof: OrderFat %zu, OrderHot %zu, OrderCold %zu bytes\n", sizeof(OrderFat), sizeof(OrderHot),
           sizeof(OrderCold));
    printf("bytes the hot loop walks over: fat %zu MB, hot %zu MB\n", n * sizeof(OrderFat) / 1000000,
           n * sizeof(OrderHot) / 1000000);
    printf("fat scan: min %g ms, median %g ms (this machine)\n", t_fat.min_ns / 1e6, t_fat.median_ns / 1e6);
    printf("hot scan: min %g ms, median %g ms (this machine)\n", t_hot.min_ns / 1e6, t_hot.median_ns / 1e6);
    printf("ratio fat / hot (median): %gx (this machine)\n", t_fat.median_ns / t_hot.median_ns);
    printf("largest order: id %" PRIu64 ", %s, created %" PRIu64 " (cold[i] read once, through the same index)\n",
           hot[best].id, cold[best].client, cold[best].created);
    printf("total notional: fat %" PRIu64 ", hot %" PRIu64 "%s", (uint64_t)total_fat, (uint64_t)total_hot,
           equal ? " (equal)\n" : " (DIFFERENT)\n");
    rc = equal ? 0 : 1;
done:
    /* no destructor in C: the three arrays are freed by hand */
    free(fat);
    free(cold);
    free(hot);
    return rc;
}
