// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 11: huge pages of 2 mib
// Build: make 11_huge_pages
#include <cstddef>
#include <iostream>

#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/mman.h>
#include <sys/resource.h>

static long minor_faults() {
    rusage u{}; getrusage(RUSAGE_SELF, &u); return u.ru_minflt;
}

// the baseline of slide 10: the same touch loop on 4 KiB pages
static long small_page_faults(std::size_t bytes) {
    char* q = static_cast<char*>(mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (q == MAP_FAILED) return -1;
    if (madvise(q, bytes, MADV_NOHUGEPAGE) != 0) std::cout << "(MADV_NOHUGEPAGE refused, baseline may be low)\n";
    long before = minor_faults();
    for (std::size_t i = 0; i < bytes; i += 4096) q[i] = 1;
    long n = minor_faults() - before;
    munmap(q, bytes);
    return n;
}
#endif

int main() {
    // the arithmetic of the figure, on every platform
    const std::size_t tlb_entries = 64;
    std::cout << "TLB reach with " << tlb_entries << " entries: " << tlb_entries * 4 << " KiB with 4 KiB pages, "
              << tlb_entries * 2 << " MiB with 2 MiB pages\n"
              << "2 MiB / 4 KiB = " << (std::size_t{2} << 20) / 4096 << ": one huge page replaces one whole page table\n\n";

#if defined(__linux__)
    std::ifstream thp("/sys/kernel/mm/transparent_hugepage/enabled");
    std::string mode;
    if (thp && std::getline(thp, mode)) std::cout << "transparent huge pages: " << mode << "\n";
    else std::cout << "transparent huge pages: no /sys file here, the hint will probably be refused\n";

    // the same 64 MiB, but ask for transparent huge pages of 2 MiB
    const std::size_t bytes = std::size_t{64} << 20;
    char* p = static_cast<char*>(mmap(nullptr, bytes,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (p == MAP_FAILED) {
        std::cout << "mmap refused: " << std::strerror(errno) << "\n";
        return 0;
    }
    if (madvise(p, bytes, MADV_HUGEPAGE) != 0)   // a hint, may be refused
        std::cout << "madvise(MADV_HUGEPAGE) refused: " << std::strerror(errno)
                  << " (on a kernel with huge pages you would see far fewer faults below)\n";
    long before = minor_faults();
    for (std::size_t i = 0; i < bytes; i += 4096) p[i] = 1;
    long huge = minor_faults() - before;         // up to 512 times fewer
    // one TLB entry now covers 2 MiB instead of 4 KiB
    munmap(p, bytes);

    long small = small_page_faults(bytes);
    std::cout << "minor faults touching 64 MiB, 4 KiB pages      : " << small << "\n"
              << "minor faults touching 64 MiB, huge pages hinted: " << huge << "\n";
    if (small > 0 && huge > 0)
        std::cout << "ratio: " << double(small) / double(huge) << "x fewer faults on this machine (512x is the limit; "
                  << "the unaligned ends of the mapping stay on 4 KiB pages)\n";
#else
    std::cout << "this sample needs Linux: it would touch 64 MiB after madvise(MADV_HUGEPAGE) and print far fewer "
              << "minor faults than the 16384 of the 4 KiB run\n";
#endif
    return 0;
}
