// Memory Performance: Cache Misses, False Sharing and NUMA - slide 10: memory bandwidth with memcpy
// Build: make 10_memcpy_bandwidth
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>
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

constexpr int kHops = 200000;                          // dependent reads per run
constexpr std::size_t kNodes = std::size_t{1} << 24;   // 16M indexes of 4 bytes: 64 MiB, bigger than any cache

// every read tells where the next one is: the core cannot start hop n + 1 before hop n arrives
static std::uint64_t follow(const std::vector<std::uint32_t>& next, int hops) {
    std::uint32_t i = 0;
    std::uint64_t sum = 0;
    for (int h = 0; h < hops; ++h) {
        i = next[i];
        sum += i;
    }
    return sum;
}

int main() {
    // one random cycle through all the nodes (the Sattolo shuffle, fixed seed): no short loops, nothing to prefetch
    std::vector<std::uint32_t> next(kNodes);
    for (std::size_t i = 0; i < kNodes; ++i) next[i] = static_cast<std::uint32_t>(i);
    std::mt19937 rng(12345);
    for (std::size_t i = kNodes - 1; i > 0; --i) {
        const std::size_t j = static_cast<std::size_t>(rng()) % i;
        std::swap(next[i], next[j]);
    }

    constexpr std::size_t kBytes = 128u << 20;          // 128 MiB
    std::vector<char> src(kBytes, 1), dst(kBytes, 0);   // already mapped
    auto copy = bench_ns([&] {                 // one read, one write stream
        std::memcpy(dst.data(), src.data(), kBytes);
        sink(static_cast<unsigned char>(dst[kBytes - 1]));
    });
    double gib_s = (kBytes / 1073741824.0) / (copy.median_ns * 1e-9);
    // latency: a chase through the same RAM, one dependent read at a time
    auto chase = bench_ns([&] { sink(follow(next, kHops)); });
    double ns_per_hop = chase.median_ns / kHops;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "bandwidth: memcpy of 128 MiB, median " << copy.median_ns / 1e6 << " ms = " << gib_s
              << " GiB per second (this machine)\n";
    std::cout << "latency  : " << kHops << " dependent reads over 64 MiB, " << ns_per_hop
              << " ns per hop (this machine)\n";
    const double chase_mib_s = (4.0 / 1048576.0) / (ns_per_hop * 1e-9);
    std::cout << "the chase delivers " << chase_mib_s << " MiB of indexes per second: the same RAM, "
              << gib_s * 1024.0 / chase_mib_s << "x less data per second than the stream\n";

    if (dst[0] != 1 || dst[kBytes - 1] != 1) {
        std::cout << "FAIL: the copy did not arrive\n";
        return 1;
    }
    return 0;
}
