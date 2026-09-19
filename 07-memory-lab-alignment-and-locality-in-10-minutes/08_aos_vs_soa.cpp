// Memory Alignment and Locality: Why Data Layout Matters - slide 8: timing the one field scan
// Build: make 08_aos_vs_soa
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
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

struct Particle {                 // AoS: 64 bytes, one cache line each
    double x, y, z, vx, vy, vz, mass, charge;
};
struct Particles {                // SoA: one contiguous array per field
    std::vector<double> x, y, z, vx, vy, vz, mass, charge;
};

int main() {
    const std::size_t n = 1000000;                 // 64 MB in each layout
    std::vector<Particle> a(n);
    Particles b;
    for (std::vector<double>* field : {&b.x, &b.y, &b.z, &b.vx, &b.vy, &b.vz, &b.mass, &b.charge}) field->resize(n);

    std::mt19937 rng(12345);                       // fixed seed: every run and every toolchain does the same work
    std::uniform_int_distribution<int> small_int(0, 9);
    for (std::size_t i = 0; i < n; ++i) {          // whole numbers, so both sums are exact and must be equal
        Particle p{};
        p.x = small_int(rng);
        p.y = small_int(rng);
        p.z = small_int(rng);
        p.mass = 1.0;
        a[i] = p;
        b.x[i] = p.x;
        b.y[i] = p.y;
        b.z[i] = p.z;
        b.mass[i] = p.mass;
    }

    auto aos = bench_ns([&] {         // sum of x: 1 cache line per element
        double s = 0; for (const Particle& p : a) s += p.x;
        sink(static_cast<std::uint64_t>(s)); });
    auto soa = bench_ns([&] {         // sum of x: 1 cache line per 8
        double s = 0; for (double v : b.x) s += v;
        sink(static_cast<std::uint64_t>(s)); });

    double sum_aos = 0, sum_soa = 0;
    for (const Particle& p : a) sum_aos += p.x;
    for (double v : b.x) sum_soa += v;
    const bool equal = static_cast<std::uint64_t>(sum_aos) == static_cast<std::uint64_t>(sum_soa);

    std::cout << "sizeof(Particle) = " << sizeof(Particle) << " bytes, n = " << n << "\n";
    std::cout << "bytes that travel for the sum of x: AoS " << n * sizeof(Particle) / 1000000 << " MB, SoA "
              << n * sizeof(double) / 1000000 << " MB\n";
    std::cout << "AoS: min " << aos.min_ns / 1e6 << " ms, median " << aos.median_ns / 1e6 << " ms (this machine)\n";
    std::cout << "SoA: min " << soa.min_ns / 1e6 << " ms, median " << soa.median_ns / 1e6 << " ms (this machine)\n";
    std::cout << "ratio AoS / SoA (median): " << aos.median_ns / soa.median_ns << "x (this machine)\n";
    std::cout << "sum of x: AoS " << sum_aos << ", SoA " << sum_soa << (equal ? " (equal)\n" : " (DIFFERENT)\n");
    return equal ? 0 : 1;
}
