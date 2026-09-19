// Inside the CPU: What We Learned About Memory - slide 3: three rules: small, contiguous, local
// Build: make 03_three_rules
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(disable : 4324)   // "structure was padded due to alignment specifier": that is the point of alignas(64)
#endif

static volatile std::uint64_t g_sink = 0;
static void sink(std::uint64_t x) { g_sink = g_sink + x; }   // keeps the result alive: the loop cannot be deleted
static void sink(double x) { sink(static_cast<std::uint64_t>(x)); }

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
    struct Loose { char tag; double price; char side; };   // 24 bytes
    struct Tight { double price; char tag; char side; };   // small: 16

    std::vector<Tight> book(1 << 20);       // contiguous: one block

    // the data and the shuffled index the slide assumes: whole numbers, so both sums are exact in a double
    for (std::size_t i = 0; i < book.size(); ++i) book[i] = Tight{static_cast<double>(i % 100), 'B', 'S'};
    std::vector<std::size_t> order(book.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::mt19937 rng(12345);                // fixed seed: every run does the same work
    std::shuffle(order.begin(), order.end(), rng);

    auto seq = bench_ns([&] {               // in order: prefetcher helps
        double s = 0; for (const Tight& t : book) s += t.price; sink(s);
    });
    auto rnd = bench_ns([&] {               // shuffled: a miss per step
        double s = 0; for (auto i : order) s += book[i].price; sink(s);
    });
    struct alignas(64) Counter { long n; }; // local: one line per thread

    // rule 1, small: same members, two orders
    std::cout << "rule 1, keep data small\n";
    std::cout << "  sizeof(Loose) = " << sizeof(Loose) << " bytes, alignof " << alignof(Loose) << "\n";
    std::cout << "  sizeof(Tight) = " << sizeof(Tight) << " bytes, alignof " << alignof(Tight) << "\n";
    std::cout << "  per 64 byte cache line: " << 64 / sizeof(Loose) << " Loose or " << 64 / sizeof(Tight) << " Tight\n";
    if (sizeof(Tight) > sizeof(Loose)) { std::cout << "FAIL: reordering made the struct bigger\n"; return 1; }

    // rule 2, contiguous: same elements, same sum, two orders of access
    double in_order = 0, shuffled = 0;
    for (const Tight& t : book) in_order += t.price;
    for (std::size_t i : order) shuffled += book[i].price;
    std::cout << "rule 2, keep it contiguous (" << book.size() << " elements, " << book.size() * sizeof(Tight) / (1024 * 1024) << " MiB)\n";
    std::cout << "  in order: min " << seq.min_ns / 1e6 << " ms, median " << seq.median_ns / 1e6 << " ms\n";
    std::cout << "  shuffled: min " << rnd.min_ns / 1e6 << " ms, median " << rnd.median_ns / 1e6 << " ms\n";
    std::cout << "  shuffled / in order, this machine: " << rnd.median_ns / seq.median_ns << "x (median)\n";
    if (in_order != shuffled) { std::cout << "FAIL: the two sums differ\n"; return 1; }
    std::cout << "  same sum both ways: " << static_cast<long long>(in_order) << "\n";

    // rule 3, local: two threads, each one writes only its own cache line
    static Counter counters[2] = {};
    const long rounds = 5'000'000;
    {
        std::thread a([&] { for (long k = 0; k < rounds; ++k) counters[0].n = counters[0].n + 1; });
        std::thread b([&] { for (long k = 0; k < rounds; ++k) counters[1].n = counters[1].n + 2; });
        a.join();
        b.join();
    }
    auto a0 = reinterpret_cast<std::uintptr_t>(&counters[0]);
    auto a1 = reinterpret_cast<std::uintptr_t>(&counters[1]);
    std::cout << "rule 3, keep it local\n";
    std::cout << "  sizeof(Counter) = " << sizeof(Counter) << ", alignof " << alignof(Counter) << "\n";
    std::cout << "  the two counters are " << (a1 - a0) << " bytes apart: one cache line each, no false sharing\n";
    std::cout << "  counters: " << counters[0].n << " and " << counters[1].n << "\n";
    if (a0 % 64 != 0 || a1 % 64 != 0 || a1 - a0 < 64) { std::cout << "FAIL: a counter is not on its own cache line\n"; return 1; }
    if (counters[0].n != rounds || counters[1].n != 2 * rounds) { std::cout << "FAIL: a count was lost\n"; return 1; }
    return 0;
}
