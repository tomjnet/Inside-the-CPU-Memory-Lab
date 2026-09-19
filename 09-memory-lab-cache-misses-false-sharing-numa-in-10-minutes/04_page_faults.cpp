// Memory Performance: Cache Misses, False Sharing and NUMA - slide 4: tlb misses and page faults
// Build: make 04_page_faults
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

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

#if defined(__linux__)
static long minor_faults() {
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return u.ru_minflt;
}
#endif

int main() {
    // the mapped buffer of the slide: the fill writes every page, so all of them are mapped before any timing
    std::vector<char> buf(std::size_t{64} << 20, 0);

    constexpr std::size_t kPage = 4096, kBytes = 64u << 20; // 16384 pages
    // fresh buffer: the first write to each page is a minor page fault
    auto fresh = bench_ns([&] {
        auto p = std::make_unique_for_overwrite<char[]>(kBytes);
        for (std::size_t i = 0; i < kBytes; i += kPage) p[i] = 1;
        sink(static_cast<unsigned char>(p[kBytes - kPage]));
    });
    // mapped buffer: no faults left, only TLB misses and cache misses
    auto mapped = bench_ns([&] {
        for (std::size_t i = 0; i < kBytes; i += kPage) buf[i] = 1;
        sink(static_cast<unsigned char>(buf[kBytes - kPage]));
    });

    const double pages = static_cast<double>(kBytes / kPage);
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "one write per 4 KiB page, " << kBytes / kPage << " pages, median of 21 runs (this machine):\n";
    std::cout << "  fresh buffer : " << std::setw(9) << fresh.median_ns / 1e6 << " ms, "
              << fresh.median_ns / pages << " ns per page (allocation, page faults, release)\n";
    std::cout << "  mapped buffer: " << std::setw(9) << mapped.median_ns / 1e6 << " ms, "
              << mapped.median_ns / pages << " ns per page (TLB misses and cache misses only)\n";
    std::cout << "  ratio        : " << fresh.median_ns / mapped.median_ns << "x\n";

#if defined(__linux__)
    // the kernel's own count: minor faults of one more fresh pass
    const long before = minor_faults();
    {
        auto p = std::make_unique_for_overwrite<char[]>(kBytes);
        for (std::size_t i = 0; i < kBytes; i += kPage) p[i] = 1;
        sink(static_cast<unsigned char>(p[kBytes - kPage]));
    }
    const long faults = minor_faults() - before;
    std::cout << "minor page faults of one fresh pass (getrusage): " << faults << ", one per page would be "
              << kBytes / kPage << "\n";
    if (faults < static_cast<long>(kBytes / kPage) / 2)
        std::cout << "  far fewer than one per page: transparent huge pages mapped 2 MiB at a time\n";
#else
    std::cout << "this sample needs Linux: the minor page fault count of one fresh pass, read with getrusage\n";
#endif
    return 0;
}
