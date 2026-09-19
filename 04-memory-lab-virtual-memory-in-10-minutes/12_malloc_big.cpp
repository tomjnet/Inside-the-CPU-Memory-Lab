// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 12: mmap under malloc
// Build: make 12_malloc_big
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>

static volatile std::uint64_t g_sink = 0;
static void sink(std::uint64_t x) { g_sink = g_sink + x; }   // keeps the pointer alive: the pair cannot be deleted

static void show(const char* name, void* p) {
    std::cout << "  " << name << "  0x" << std::hex << reinterpret_cast<std::uintptr_t>(p) << std::dec << '\n';
}

// Average cost of one malloc and free pair of the given size, in nanoseconds.
static double pair_ns(std::size_t bytes, int pairs) {
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < pairs; ++i) {
        void* p = std::malloc(bytes);
        sink(reinterpret_cast<std::uintptr_t>(p));
        std::free(p);
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / pairs;
}

static int global_anchor = 0;               // something in the data region, to measure distances from

int main() {
    // small blocks come from the heap, big ones from their own mmap
    void* small = std::malloc(64);           // heap: no system call
    void* big = std::malloc(256u << 20);     // 256 MiB: one mmap inside
    show("small", small);                    // just above the data
    show("big  ", big);                      // far away, near the libs
    if (small == nullptr || big == nullptr) {
        std::cout << "malloc returned nullptr: this machine refused 256 MiB of address space\n";
        std::free(big);
        std::free(small);
        return 0;
    }

    const auto data_at = reinterpret_cast<std::uintptr_t>(&global_anchor);
    const auto small_at = reinterpret_cast<std::uintptr_t>(small);
    const auto big_at = reinterpret_cast<std::uintptr_t>(big);
    const double mib = 1024.0 * 1024.0;
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  distance data to small  " << static_cast<double>(small_at > data_at ? small_at - data_at : data_at - small_at) / mib << " MiB\n";
    std::cout << "  distance data to big    " << static_cast<double>(big_at > data_at ? big_at - data_at : data_at - big_at) / mib << " MiB\n";

    std::free(big);                          // munmap: back to the kernel
    std::free(small);                        // back to the free list

    const double small_ns = pair_ns(64, 200000);
    const double big_ns = pair_ns(std::size_t{256} << 20, 2000);
    std::cout << "\nmalloc plus free, average per pair on this machine:\n";
    std::cout << "  64 bytes   " << small_ns << " ns  (free list, no system call)\n";
    std::cout << "  256 MiB    " << big_ns << " ns  (a system call to map and one to unmap)\n";
    std::cout << "  ratio      " << big_ns / small_ns << "x\n";
#if defined(__linux__)
    std::cout << "glibc maps blocks from 128 KiB by default (M_MMAP_THRESHOLD), and the threshold adapts\n";
#else
    std::cout << "this is not Linux: the C library here has its own threshold, the idea is the same\n";
#endif
    return 0;
}
