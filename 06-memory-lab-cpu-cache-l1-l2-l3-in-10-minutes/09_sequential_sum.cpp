// CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 9: sequential sum: the baseline
// Build: make 09_sequential_sum
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
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

int main() {
    const std::size_t n = std::size_t{1} << 22;   // 4M ints = 16 MiB
    std::vector<int> data(n, 1);            // bigger than L1 and L2
    std::vector<std::uint32_t> order(n);    // order[i] = i for now
    std::iota(order.begin(), order.end(), 0u);

    auto sum_by = [&](const std::vector<std::uint32_t>& idx) {
        long long sum = 0;
        for (std::uint32_t i : idx) sum += data[i];   // same code for
        return sum;                                   // both orders
    };
    auto seq = bench_ns([&] { sink(sum_by(order)); });
    // 0, 1, 2, 3: one miss per 16 ints, the prefetcher hides even that

    const double per_int = seq.median_ns / static_cast<double>(n);
    std::cout << "sequential sum through an index array, this machine\n";
    std::cout << "  data : " << n << " ints, " << n * sizeof(int) / (1024 * 1024) << " MiB\n";
    std::cout << "  order: " << order.front() << ", " << order[1] << ", " << order[2] << ", " << order[3] << " to "
              << order.back() << "\n";
    std::cout << "  min " << seq.min_ns / 1e6 << " ms, median " << seq.median_ns / 1e6 << " ms, " << per_int
              << " ns per int\n";
    std::cout << "  cache lines of data touched: " << n * sizeof(int) / 64 << ", one new line per 16 reads\n";
    std::cout << "this is the baseline: 10_shuffled_sum runs the same lambda with the same indexes, shuffled\n";

    if (sum_by(order) != static_cast<long long>(n)) {
        std::cout << "FAIL: the sum is wrong\n";
        return 1;
    }
    std::cout << "check: the sum is exact\n";
    return 0;
}
