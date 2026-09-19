// Memory Performance: Cache Misses, False Sharing and NUMA - slide 9: the fix: alignas(64)
// Build: make 09_alignas_fix
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

struct Counters {                  // slide 8: 16 bytes, both counters in one cache line
    std::atomic<std::uint64_t> a{0};
    std::atomic<std::uint64_t> b{0};
};
struct alignas(64) OneLine { Counters c; };   // as in 08_false_sharing: c starts on a line boundary

template <class Work>
static void run_two_threads(Work& work, std::atomic<std::uint64_t>& x, std::atomic<std::uint64_t>& y) {
    std::thread t1(work, std::ref(x));
    std::thread t2(work, std::ref(y));
    t1.join(); t2.join();
}

int main() {
    auto work = [](std::atomic<std::uint64_t>& n) {
        for (int i = 0; i < kIters; ++i) n.fetch_add(1);  // a write
    };
    OneLine line;
    Counters& c = line.c;
    auto shared = bench_ns([&] { run_two_threads(work, c.a, c.b); });

    struct Padded {                    // 128 bytes: one cache line each
        alignas(64) std::atomic<std::uint64_t> a{0};
        alignas(64) std::atomic<std::uint64_t> b{0};
    };
    static_assert(sizeof(Padded) == 128 && alignof(Padded) == 64);
    Padded p;
    auto padded = bench_ns([&] { run_two_threads(work, p.a, p.b); });
    // same work, no bouncing line: often several times faster
    double ratio = shared.median_ns / padded.median_ns;

    std::cout << "sizeof(Counters) = " << sizeof(Counters) << " bytes, sizeof(Padded) = " << sizeof(Padded)
              << " bytes, alignof(Padded) = " << alignof(Padded) << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "two threads, " << kIters << " fetch_add each, median of 21 runs (this machine):\n";
    std::cout << "  one shared line     : " << shared.median_ns / 1e6 << " ms\n";
    std::cout << "  one line per counter: " << padded.median_ns / 1e6 << " ms\n";
    std::cout << "  ratio               : " << ratio << "x (a virtual machine that shares cores can hide it)\n";

    // correctness: both layouts count every update of 3 warm up runs plus 21 timed runs
    const std::uint64_t expected = 24ull * static_cast<std::uint64_t>(kIters);
    if (c.a.load() != expected || c.b.load() != expected || p.a.load() != expected || p.b.load() != expected) {
        std::cout << "FAIL: a counter lost updates\n";
        return 1;
    }
    std::cout << "all four counters = " << expected << ": same result, different layout\n";
    return 0;
}
