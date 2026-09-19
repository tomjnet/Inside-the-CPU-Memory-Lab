// Build a Simple Memory Allocator: Understanding malloc and new - slide 4: bump allocator: the fastest one
// Build: make 04_bump_allocator
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

// ---- slide 3: the arena and the interface ----
constexpr std::size_t ARENA = 4096;          // one page, fixed forever
constexpr std::size_t ALIGN = 16;            // what malloc promises
alignas(16) static unsigned char arena[ARENA];

// round n up to a multiple of 16: 1 -> 16, 16 -> 16, 17 -> 32
constexpr std::size_t align_up(std::size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

void* allocate(std::size_t size);            // nullptr when it is full
void  deallocate(void* ptr);                 // no size: we store it

// ---- slide 4: bump allocator ----
static std::size_t top = 0;                  // first free byte
void* allocate(std::size_t size) {           // O(1): add and compare
    std::size_t need = align_up(size);
    if (need > ARENA - top) return nullptr;  // arena full
    void* p = arena + top;
    top += need;                             // the bump
    return p;
}
void reset() { top = 0; }                    // frees everything: O(1)

static volatile std::uint64_t g_sink = 0;
static void sink(std::uint64_t x) { g_sink = g_sink + x; }   // keeps the result alive: the loop cannot be deleted

struct BenchResult { double min_ns; double median_ns; };

template <class F>
static BenchResult bench_ns(F&& f, int warmup = 3, int repeats = 21) {
    for (int i = 0; i < warmup; ++i) f();                     // warm up: caches, branch predictor, page faults
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(repeats));
    for (int i = 0; i < repeats; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        f();
        auto t1 = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }
    std::sort(samples.begin(), samples.end());
    return BenchResult{samples.front(), samples[samples.size() / 2]};
}

static void* held[128];                      // the malloc side keeps its blocks alive, as the bump side does

int main() {
    bool ok = true;
    std::cout << "three allocations, top after each one:\n";
    const std::size_t sizes[] = {100, 200, 24};
    for (std::size_t n : sizes) {
        void* p = allocate(n);
        auto off = static_cast<std::size_t>(static_cast<unsigned char*>(p) - arena);
        std::cout << "  allocate(" << n << ") -> arena + " << off << ", top = " << top << "\n";
        if (p == nullptr || off % ALIGN != 0) ok = false;
    }
    if (top != 352) ok = false;              // 112 + 208 + 32, the numbers of the figure

    std::size_t count = 0;
    while (allocate(24) != nullptr) ++count; // fill the rest: 32 bytes per block
    std::cout << "then " << count << " more blocks of 24 bytes fit, top = " << top << ", next allocate: nullptr\n";
    reset();
    std::cout << "reset(): top = " << top << ", the whole arena is free again in one step\n\n";

    // 128 blocks of 24 bytes, release them all, 1000 times: bump + reset against malloc + free
    const int rounds = 1000;
    BenchResult bump = bench_ns([&] {
        for (int r = 0; r < rounds; ++r) {
            void* last = nullptr;
            for (int i = 0; i < 128; ++i) last = allocate(24);
            sink(reinterpret_cast<std::uintptr_t>(last));
            reset();
        }
    });
    BenchResult libc = bench_ns([&] {
        for (int r = 0; r < rounds; ++r) {
            for (int i = 0; i < 128; ++i) held[i] = std::malloc(24);
            sink(reinterpret_cast<std::uintptr_t>(held[127]));
            for (int i = 0; i < 128; ++i) std::free(held[i]);
        }
    });
    double per_bump = bump.median_ns / (rounds * 128.0);
    double per_libc = libc.median_ns / (rounds * 128.0);
    std::cout << "this machine, median per block: bump " << per_bump << " ns, malloc + free " << per_libc << " ns";
    if (per_bump > 0.0) std::cout << ", ratio " << per_libc / per_bump;
    std::cout << "\n(bump cannot free one block: that is what it pays for the speed)\n";
    return ok ? 0 : 1;
}
