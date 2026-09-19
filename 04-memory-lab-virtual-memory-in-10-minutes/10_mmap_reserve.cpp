// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 10: reserve 1 GiB with mmap
// Build: make 10_mmap_reserve
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>

#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#endif

// reserve 1 GiB of address space: one system call, no RAM yet
const std::size_t kGiB = std::size_t{1} << 30;

#if defined(__linux__)
char* reserve_gib() {
    void* p = mmap(nullptr, kGiB,               // anywhere, 1 GiB
                   PROT_READ | PROT_WRITE,      // readable, writable
                   MAP_PRIVATE | MAP_ANONYMOUS, // no file behind it
                   -1, 0);
    if (p == MAP_FAILED) return nullptr;        // errno says why
    return static_cast<char*>(p);               // zero pages, lazily
}

// The same call for any size, timed: the cost is the system call, not the length.
static void time_reserve(const char* label, std::size_t bytes) {
    const auto t0 = std::chrono::steady_clock::now();
    void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    const auto t1 = std::chrono::steady_clock::now();
    if (p == MAP_FAILED) {
        std::cout << "  " << label << "  refused: " << std::strerror(errno)
                  << " (the overcommit check of the kernel said no)\n";
        return;
    }
    std::cout << "  " << label << "  " << std::chrono::duration<double, std::nano>(t1 - t0).count()
              << " ns on this machine\n";
    munmap(p, bytes);
}
#endif

int main() {
    std::cout << "1 GiB = " << kGiB << " bytes = " << kGiB / 4096 << " pages of 4 KiB\n";

#if defined(__linux__)
    char* base = reserve_gib();
    if (base == nullptr) {
        std::cout << "mmap refused 1 GiB: " << std::strerror(errno) << '\n';
        std::cout << "on a normal Linux it returns an address at once and assigns no RAM\n";
        return 0;
    }
    std::cout << "mmap returned 0x" << std::hex << reinterpret_cast<std::uintptr_t>(base) << std::dec
              << ": a new region in /proc/self/maps, no frame of RAM behind it yet\n";
    std::cout << "first byte reads as " << static_cast<int>(base[0]) << ": anonymous pages start as zeros\n";
    munmap(base, kGiB);                         // give the range back

    std::cout << "\nthe cost does not follow the size:\n";
    time_reserve("  1 MiB", std::size_t{1} << 20);
    time_reserve("  1 GiB", kGiB);
    time_reserve(" 16 GiB", std::size_t{16} << 30);
    time_reserve("  4 TiB", std::size_t{4} << 40);
    std::cout << "a refusal above is overcommit at work: the kernel promises a lot, not everything\n";
#else
    std::cout << "this sample needs Linux: mmap would reserve 1 GiB of address space in one system call,\n";
    std::cout << "in about the same time as 1 MiB, because no RAM is assigned until a page is touched\n";
#endif
    return 0;
}
