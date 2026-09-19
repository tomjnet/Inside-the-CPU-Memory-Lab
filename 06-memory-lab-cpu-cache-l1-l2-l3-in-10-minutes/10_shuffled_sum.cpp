// CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 10: shuffled index: same O(n), different time
// Build: make 10_shuffled_sum
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
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

int main() {
    // the baseline of slide 9, unchanged
    const std::size_t n = std::size_t{1} << 22;   // 4M ints = 16 MiB
    std::vector<int> data(n, 1);
    for (std::size_t i = 0; i < n; ++i) data[i] = static_cast<int>(i % 7);   // not all equal: the sum is a real check
    std::vector<std::uint32_t> order(n);
    std::iota(order.begin(), order.end(), 0u);
    auto sum_by = [&](const std::vector<std::uint32_t>& idx) {
        long long sum = 0;
        for (std::uint32_t i : idx) sum += data[i];
        return sum;
    };
    auto seq = bench_ns([&] { sink(sum_by(order)); });

    std::vector<std::uint32_t> shuffled = order;   // same n indexes
    std::mt19937 rng(42);                          // fixed seed
    std::shuffle(shuffled.begin(), shuffled.end(), rng);

    auto rnd = bench_ns([&] { sink(sum_by(shuffled)); });
    // same n reads, same sum, same O(n): only the order changed
    std::cout << "random / sequential: "
              << rnd.median_ns / seq.median_ns << "x\n";
    // typical: several times slower, more as the array grows

    const double dn = static_cast<double>(n);
    std::cout << "(this machine, " << n << " ints, " << n * sizeof(int) / (1024 * 1024) << " MiB)\n";
    std::cout << "  order   : " << order[0] << ", " << order[1] << ", " << order[2] << ", " << order[3]
              << "  median " << seq.median_ns / 1e6 << " ms, " << seq.median_ns / dn << " ns per int\n";
    std::cout << "  shuffled: " << shuffled[0] << ", " << shuffled[1] << ", " << shuffled[2] << ", " << shuffled[3]
              << "  median " << rnd.median_ns / 1e6 << " ms, " << rnd.median_ns / dn << " ns per int\n";

    // how often the next read stays on the cache line of the previous one
    auto same_line = [](const std::vector<std::uint32_t>& idx) {
        std::size_t same = 0;
        for (std::size_t i = 1; i < idx.size(); ++i)
            if (idx[i] / 16 == idx[i - 1] / 16) ++same;
        return 100.0 * static_cast<double>(same) / static_cast<double>(idx.size() - 1);
    };
    std::cout << "  next read on the same 64 byte line: order " << same_line(order) << " %, shuffled "
              << same_line(shuffled) << " %\n";

    const long long a = sum_by(order);
    const long long b = sum_by(shuffled);
    if (a != b) {
        std::cout << "FAIL: the two orders give different sums\n";
        return 1;
    }
    std::cout << "check: same sum in both orders (" << a << "), same number of reads, only the order changed\n";
    return 0;
}
