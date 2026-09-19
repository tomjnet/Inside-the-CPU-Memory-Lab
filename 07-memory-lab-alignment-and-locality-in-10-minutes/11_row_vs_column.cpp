// Memory Alignment and Locality: Why Data Layout Matters - slide 11: timing row versus column
// Build: make 11_row_vs_column
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

static volatile std::uint64_t g_sink = 0;
static void sink(std::uint64_t x) { g_sink = g_sink + x; }   // keeps the result alive: the loop cannot be deleted

struct BenchResult { double min_ns; double median_ns; };

// The slide calls bench_ns(f); the defaults here are 2 warm up runs and 11 repeats so the slow walk stays short.
template <class F>
static BenchResult bench_ns(F&& f, int warmup = 2, int repeats = 11) {
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
    constexpr std::size_t N = 2048;          // N x N ints: 16 MiB
    std::vector<int> m(N * N, 1);            // row major: m[r * N + c]
    auto by_row = bench_ns([&] {             // stride 4 bytes: sequential
        std::uint64_t s = 0;
        for (std::size_t r = 0; r < N; ++r)
            for (std::size_t c = 0; c < N; ++c) s += m[r * N + c];
        sink(s); });
    auto by_col = bench_ns([&] {             // stride 8 KiB: a miss each
        std::uint64_t s = 0;
        for (std::size_t c = 0; c < N; ++c)
            for (std::size_t r = 0; r < N; ++r) s += m[r * N + c];
        sink(s); });

    // Both walks must add every cell exactly once: the sums are equal, only the order differs.
    std::uint64_t sum_row = 0, sum_col = 0;
    for (std::size_t r = 0; r < N; ++r)
        for (std::size_t c = 0; c < N; ++c) sum_row += static_cast<std::uint64_t>(m[r * N + c]);
    for (std::size_t c = 0; c < N; ++c)
        for (std::size_t r = 0; r < N; ++r) sum_col += static_cast<std::uint64_t>(m[r * N + c]);

    std::cout << "matrix " << N << " x " << N << " ints = " << N * N * sizeof(int) / (1024 * 1024) << " MiB, row major\n";
    std::cout << "stride of the inner loop: by row " << sizeof(int) << " bytes, by column " << N * sizeof(int) << " bytes\n";
    std::cout << "by row:    min " << by_row.min_ns / 1e6 << " ms, median " << by_row.median_ns / 1e6 << " ms (this machine)\n";
    std::cout << "by column: min " << by_col.min_ns / 1e6 << " ms, median " << by_col.median_ns / 1e6 << " ms (this machine)\n";
    std::cout << "ratio column / row (median): " << by_col.median_ns / by_row.median_ns << "x (this machine)\n";
    std::cout << "sums: " << sum_row << " and " << sum_col << (sum_row == sum_col ? " (equal)\n" : " (DIFFERENT)\n");
    return sum_row == sum_col ? 0 : 1;
}
