// Stack vs Heap: Where Does Your Data Actually Live? - slide 12: lab: a million new and delete pairs
// Build: make 12_time_new_delete
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

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

struct Point { int x; int y; };

static constexpr int N = 1'000'000;

// The pointer is stored through a volatile: it escapes, so the compiler cannot remove the new and delete pair.
static Point* volatile g_keep = nullptr;
static void keep(Point* p) { g_keep = p; }

int main() {
    auto heap = bench_ns([] {                 // N = 1'000'000
        for (int i = 0; i < N; ++i) {
            Point* p = new Point{i, i};       // allocator call, header
            keep(p);                          // escapes: pair not elided
            sink(p->x + p->y);
            delete p;                         // allocator call again
        }
    });
    auto stack = bench_ns([] {
        for (int i = 0; i < N; ++i) { Point p{i, i}; sink(p.x + p.y); }
    });                                       // same frame slot: no call

    std::cout << "one million objects of " << sizeof(Point) << " B, minimum and median of 21 runs, this machine\n";
    std::cout << "  heap  (new + delete): min " << heap.min_ns / N << " ns per object, median "
              << heap.median_ns / N << " ns\n";
    std::cout << "  stack (a local)     : min " << stack.min_ns / N << " ns per object, median "
              << stack.median_ns / N << " ns\n";
    if (stack.median_ns > 0.0) {
        std::cout << "  ratio heap / stack, this machine: " << heap.median_ns / stack.median_ns << "x\n";
    }
    std::cout << "\nThe stack loop still pays for the volatile sink; the heap loop pays for it too, plus two\n";
    std::cout << "allocator calls. This is the best case for the heap: one thread, one size, freed at once.\n";
    std::cout << "Take timings on real Linux hardware; a virtual machine shares its cores.\n";
    return 0;
}
