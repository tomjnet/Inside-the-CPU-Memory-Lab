// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 10: count minor faults with getrusage
// Build: make 10_count_faults
#include <cstddef>
#include <iostream>

#if defined(__linux__)
#include <cerrno>
#include <chrono>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>

long minor_faults() {                        // ru_majflt: from disk
    rusage u{}; getrusage(RUSAGE_SELF, &u); return u.ru_minflt;
}

static double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
#endif

int main() {
    const std::size_t expected = (std::size_t{64} << 20) / 4096;
    std::cout << "64 MiB / 4 KiB = " << expected << " pages: expect about that many minor faults on the first touch\n";

#if defined(__linux__)
    const std::size_t pages = 16384;             // 64 MiB of 4 KiB pages
    char* p = static_cast<char*>(mmap(nullptr, pages * 4096,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (p == MAP_FAILED) {
        std::cout << "mmap refused: " << std::strerror(errno) << "\n";
        return 0;
    }
    // not on the slide: ask for 4 KiB pages, so a kernel with transparent huge pages set to "always"
    // still gives one fault per page and the count is exact
    if (madvise(p, pages * 4096, MADV_NOHUGEPAGE) != 0)
        std::cout << "madvise(MADV_NOHUGEPAGE) refused: " << std::strerror(errno) << " (the count may be lower)\n";

    auto t0 = std::chrono::steady_clock::now();
    long before = minor_faults();
    for (std::size_t i = 0; i < pages; ++i) p[i * 4096] = 1;
    long first = minor_faults() - before;        // about one per page
    double first_ms = ms_since(t0);

    t0 = std::chrono::steady_clock::now();
    before = minor_faults();
    for (std::size_t i = 0; i < pages; ++i) p[i * 4096] = 2;
    long second = minor_faults() - before;       // about zero: mapped
    double second_ms = ms_since(t0);

    std::cout << "first touch : " << first << " minor faults, " << first_ms << " ms (this machine)\n"
              << "second touch: " << second << " minor faults, " << second_ms << " ms (this machine)\n";
    if (second_ms > 0.0) std::cout << "ratio       : " << first_ms / second_ms << "x, the kernel filling page tables\n";
    std::cout << "check bytes : " << int(p[0]) + int(p[(pages - 1) * 4096]) << "\n";
    munmap(p, pages * 4096);
#else
    std::cout << "this sample needs Linux: it would mmap 64 MiB, touch one byte per page and print about " << expected
              << " minor faults from getrusage on the first pass and about 0 on the second\n";
#endif
    return 0;
}
