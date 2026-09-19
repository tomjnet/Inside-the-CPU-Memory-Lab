// Memory Performance: Cache Misses, False Sharing and NUMA - slide 8: false sharing, measured
// Build: make 08_false_sharing
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(disable : 4324)   // "structure was padded due to alignment specifier": that is the point of alignas(64)
#endif

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

constexpr int kIters = 1000000;    // writes per thread and per run

struct Counters {                  // 16 bytes: both in ONE cache line
    std::atomic<std::uint64_t> a{0};
    std::atomic<std::uint64_t> b{0};
};

// The slide writes "Counters c;". A 16 byte object with 8 byte alignment can straddle two cache lines one time out
// of eight; this wrapper starts c on a line boundary, so a and b always share one line and every run shows the effect.
struct alignas(64) OneLine { Counters c; };

int main() {
    OneLine line;
    Counters& c = line.c;
    auto work = [](std::atomic<std::uint64_t>& n) {
        for (int i = 0; i < kIters; ++i) n.fetch_add(1);  // a write
    };
    auto shared = bench_ns([&] {       // no data is shared, the line is
        std::thread t1(work, std::ref(c.a));   // core 1 owns the line
        std::thread t2(work, std::ref(c.b));   // core 2 takes it back
        t1.join(); t2.join(); });

    const auto line_a = reinterpret_cast<std::uintptr_t>(&c.a) / 64;
    const auto line_b = reinterpret_cast<std::uintptr_t>(&c.b) / 64;
    std::cout << "sizeof(Counters) = " << sizeof(Counters) << " bytes, a and b in the same 64 byte line: "
              << (line_a == line_b ? "yes" : "no") << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "two threads, " << kIters << " fetch_add each, one counter per thread (this machine):\n";
    std::cout << "  median " << shared.median_ns / 1e6 << " ms per run, "
              << shared.median_ns / kIters << " ns per write\n";
    std::cout << "09_alignas_fix runs the same work with one cache line per counter and prints the ratio\n";

    // correctness: 3 warm up runs plus 21 timed runs, nothing lost
    const std::uint64_t expected = 24ull * static_cast<std::uint64_t>(kIters);
    if (c.a.load() != expected || c.b.load() != expected) {
        std::cout << "FAIL: a counter lost updates\n";
        return 1;
    }
    std::cout << "a = " << c.a.load() << ", b = " << c.b.load() << ": every update counted\n";
    return 0;
}
