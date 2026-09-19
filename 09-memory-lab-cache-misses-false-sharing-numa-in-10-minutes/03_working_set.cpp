// Memory Performance: Cache Misses, False Sharing and NUMA - slide 3: cache misses: the working set
// Build: make 03_working_set
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <random>
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

static double g_first_ns = 0.0;   // the smallest working set is the reference for the ratio column

static void report(std::size_t kib, double ns_per_read) {
    if (g_first_ns == 0.0) g_first_ns = ns_per_read;
    std::cout << "  " << std::setw(7) << kib << " KiB working set: " << std::fixed << std::setprecision(2)
              << std::setw(7) << ns_per_read << " ns per read, " << std::setprecision(1)
              << ns_per_read / g_first_ns << "x the first row (this machine)\n";
}

int main() {
    // the same one million random indexes for every size: fixed seed, so every run does the same work
    std::mt19937 rng(12345);
    std::vector<std::uint32_t> idx(1000000);
    for (std::uint32_t& i : idx) i = static_cast<std::uint32_t>(rng());

    std::cout << "one million random reads, median of 21 runs:\n";

    // one million random reads each time: only the working set grows
    for (std::size_t kib : {16u, 512u, 8192u, 131072u}) {
        std::vector<std::uint32_t> data(kib * 1024 / 4, 1);
        const std::size_t mask = data.size() - 1;     // power of two
        auto r = bench_ns([&] {
            std::uint64_t sum = 0;
            for (std::uint32_t i : idx) sum += data[i & mask];
            sink(sum);
        });
        report(kib, r.median_ns / idx.size());        // ns per read
    }
    // 16 KiB lives in L1; 128 MiB misses every cache level

    std::cout << "the reads are independent, so the core overlaps several misses: the last row stays\n"
              << "below the 100 ns latency of RAM. 10_memcpy_bandwidth chases pointers and pays it in full.\n";
    return 0;
}
