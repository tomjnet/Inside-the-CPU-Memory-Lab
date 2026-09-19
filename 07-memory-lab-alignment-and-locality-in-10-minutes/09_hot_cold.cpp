// Memory Alignment and Locality: Why Data Layout Matters - slide 9: hot and cold splitting
// Build: make 09_hot_cold
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
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

struct OrderFat {                 // 96 bytes: the scan drags it all
    std::uint64_t id; double price; std::uint32_t qty;   // hot
    char client[64]; std::uint64_t created;              // cold
};
struct OrderHot {                 // 24 bytes: what the loop reads
    std::uint64_t id; double price; std::uint32_t qty;
};
struct OrderCold {                // 72 bytes: read once per report
    char client[64]; std::uint64_t created;
};

int main() {
    std::vector<OrderHot>  hot;       // hot[i] and cold[i]: same order
    std::vector<OrderCold> cold;      // the index is the link

    const std::size_t n = 500000;
    std::vector<OrderFat> fat(n);
    hot.resize(n);
    cold.resize(n);
    std::mt19937 rng(12345);                       // fixed seed: every run and every toolchain does the same work
    std::uniform_int_distribution<int> price(1, 500), qty(1, 100);
    for (std::size_t i = 0; i < n; ++i) {          // whole numbers, so both totals are exact and must be equal
        OrderFat o{};
        o.id = i;
        o.price = price(rng);
        o.qty = static_cast<std::uint32_t>(qty(rng));
        const std::string name = "client-" + std::to_string(i % 1000);
        std::copy(name.begin(), name.end(), std::begin(o.client));   // o{} zeroed the array: the text stays terminated
        o.created = 1700000000u + i;
        fat[i] = o;
        hot[i] = OrderHot{o.id, o.price, o.qty};
        std::copy(std::begin(o.client), std::end(o.client), std::begin(cold[i].client));
        cold[i].created = o.created;
    }

    // The hot loop: the notional of the whole book, price times quantity. It never reads client or created.
    double total_fat = 0, total_hot = 0;
    auto t_fat = bench_ns([&] {
        double s = 0;
        for (const OrderFat& o : fat) s += o.price * o.qty;
        total_fat = s;
        sink(static_cast<std::uint64_t>(s)); });
    auto t_hot = bench_ns([&] {
        double s = 0;
        for (const OrderHot& o : hot) s += o.price * o.qty;
        total_hot = s;
        sink(static_cast<std::uint64_t>(s)); });
    const bool equal = static_cast<std::uint64_t>(total_fat) == static_cast<std::uint64_t>(total_hot);

    // The cold path: one report line, found through the shared index.
    std::size_t best = 0;
    for (std::size_t i = 1; i < n; ++i)
        if (hot[i].price * hot[i].qty > hot[best].price * hot[best].qty) best = i;

    std::cout << "sizeof: OrderFat " << sizeof(OrderFat) << ", OrderHot " << sizeof(OrderHot) << ", OrderCold "
              << sizeof(OrderCold) << " bytes\n";
    std::cout << "bytes the hot loop walks over: fat " << n * sizeof(OrderFat) / 1000000 << " MB, hot "
              << n * sizeof(OrderHot) / 1000000 << " MB\n";
    std::cout << "fat scan: min " << t_fat.min_ns / 1e6 << " ms, median " << t_fat.median_ns / 1e6 << " ms (this machine)\n";
    std::cout << "hot scan: min " << t_hot.min_ns / 1e6 << " ms, median " << t_hot.median_ns / 1e6 << " ms (this machine)\n";
    std::cout << "ratio fat / hot (median): " << t_fat.median_ns / t_hot.median_ns << "x (this machine)\n";
    std::cout << "largest order: id " << hot[best].id << ", " << cold[best].client << ", created " << cold[best].created
              << " (cold[i] read once, through the same index)\n";
    std::cout << "total notional: fat " << static_cast<std::uint64_t>(total_fat) << ", hot "
              << static_cast<std::uint64_t>(total_hot) << (equal ? " (equal)\n" : " (DIFFERENT)\n");
    return equal ? 0 : 1;
}
