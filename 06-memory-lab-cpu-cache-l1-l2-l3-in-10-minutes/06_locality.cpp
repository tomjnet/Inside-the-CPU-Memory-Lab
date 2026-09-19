// CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 6: spatial and temporal locality
// Build: make 06_locality
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
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

long long sum_stride(const std::vector<int>& a, std::size_t step) {
    long long sum = 0;              // temporal: sum and i are reused
    for (std::size_t i = 0; i < a.size(); i += step)
        sum += a[i];                // spatial: a[i + 1] sits next door
    return sum;
}

int main() {
    const std::size_t n = std::size_t{1} << 24;          // 16M ints = 64 MiB: far bigger than any L2
    std::vector<int> data(n, 1);

    // step 1:  16 ints per cache line, at worst 1 miss per 16 reads
    // step 16: every read lands on a new line, 64 bytes for 4 used
    auto dense  = bench_ns([&] { sink(sum_stride(data, 1)); });
    auto sparse = bench_ns([&] { sink(sum_stride(data, 16)); });
    // per int read, step 16 is often several times slower

    const double dense_reads = static_cast<double>(n);
    const double sparse_reads = static_cast<double>(n / 16);
    const double dense_per = dense.median_ns / dense_reads;
    const double sparse_per = sparse.median_ns / sparse_reads;

    std::cout << "sum_stride over " << n << " ints (" << n * sizeof(int) / (1024 * 1024) << " MiB), this machine\n";
    std::cout << "  step 1 : " << dense_reads << " reads, median " << dense.median_ns / 1e6 << " ms, "
              << dense_per << " ns per int read\n";
    std::cout << "  step 16: " << sparse_reads << " reads, median " << sparse.median_ns / 1e6 << " ms, "
              << sparse_per << " ns per int read\n";
    std::cout << "  per int read, step 16 / step 1: " << sparse_per / dense_per << "x (this machine)\n";
    std::cout << "both walks touch the same " << n / 16 << " cache lines: step 1 uses 16 ints of each, step 16 one\n";

    const bool ok = sum_stride(data, 1) == static_cast<long long>(n) &&
                    sum_stride(data, 16) == static_cast<long long>(n / 16);
    if (!ok) {
        std::cout << "FAIL: a sum is wrong\n";
        return 1;
    }
    std::cout << "check: both sums are exact\n";
    return 0;
}
