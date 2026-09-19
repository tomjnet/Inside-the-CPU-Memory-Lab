// CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 11: find the edges of L1, L2 and L3
// Build: make 11_size_sweep
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

static volatile std::uint64_t g_sink = 0;
static void sink(long long x) { g_sink = g_sink + static_cast<std::uint64_t>(x); }   // keeps the result alive

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

// 0 .. n-1 in a random order, fixed seed: every run and every toolchain does the same kind of work
static std::vector<std::uint32_t> shuffled_indexes(std::size_t n) {
    std::vector<std::uint32_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0u);
    std::mt19937 rng(42);
    std::shuffle(idx.begin(), idx.end(), rng);
    return idx;
}

static long long sum_by(const std::vector<int>& data, const std::vector<std::uint32_t>& idx) {
    long long sum = 0;
    for (std::uint32_t i : idx) sum += data[i];
    return sum;
}

int main() {
    bool ok = true;
    std::cout << "random walk, time per int by array size (this machine):\n";

    // same random walk, four array sizes: where does it stop fitting?
    for (std::size_t kib : {16u, 256u, 4096u, 65536u}) {
        std::size_t n = kib * 1024 / sizeof(int);
        std::vector<int> data(n, 1);
        std::vector<std::uint32_t> idx = shuffled_indexes(n);
        auto t = bench_ns([&] { sink(sum_by(data, idx)); }, 1, 5);
        std::cout << kib << " KiB: "
                  << t.median_ns / static_cast<double>(n)
                  << " ns per int\n";
        if (sum_by(data, idx) != static_cast<long long>(n)) ok = false;
    }
    // typical homes: 16 KiB in L1, 256 KiB in L2, 4 MiB in L3, then RAM

    std::cout << "\nhow to read it: the cost is flat while the array fits a level and steps up at each edge.\n";
    std::cout << "compare the steps with your sizes: 08_cache_sizes or lscpu --caches (typical: 48K, 2048K, 8M to 64M)\n";
    std::cout << "the index array uses cache too, so the edges are soft; in a VM the L3 is shared and the numbers jump\n";

    if (!ok) {
        std::cout << "FAIL: a sum is wrong\n";
        return 1;
    }
    std::cout << "check: every sum is exact\n";
    return 0;
}
