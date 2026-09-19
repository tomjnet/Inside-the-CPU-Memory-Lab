// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 11: touch 16 pages, read the resident size
// Build: make 11_touch_pages
#include <cstddef>
#include <iostream>

#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sys/mman.h>

// resident pages of this process: the second field of statm
long resident_pages() {
    std::ifstream statm("/proc/self/statm");
    long size = 0, resident = 0;
    statm >> size >> resident;              // both in 4 KiB pages
    return resident;
}
#endif

// touch 16 pages out of 262144: one page fault each
void touch(char* base) {
    for (std::size_t i = 0; i < 16; ++i)
        base[i * 4096] = 1;                 // first write maps a page
}

#if defined(__linux__)
// The first field of the same file: the whole address space, in pages.
static long total_pages() {
    std::ifstream statm("/proc/self/statm");
    long size = 0;
    statm >> size;
    return size;
}

static void report(const char* when, long total0, long resident0) {
    std::cout << "  " << when << "  total " << total_pages() - total0 << " pages, resident "
              << resident_pages() - resident0 << " pages (change since the start)\n";
}
#endif

int main() {
#if defined(__linux__)
    const std::size_t kGiB = std::size_t{1} << 30;
    std::cout << "reading /proc/self/statm, in pages of 4 KiB\n" << std::flush;
    (void)resident_pages();                 // warm up: the first use of a stream touches pages of its own
    const long total0 = total_pages();
    const long resident0 = resident_pages();
    std::cout << "start: total " << total0 << " pages, resident " << resident0 << " pages of 4 KiB\n";
    if (total0 == 0) {
        std::cout << "/proc/self/statm is not readable here: on a normal Linux the lines below show the change\n";
    }

    void* p = mmap(nullptr, kGiB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        std::cout << "mmap refused 1 GiB: " << std::strerror(errno) << '\n';
        std::cout << "on a normal Linux: total grows by 262144 pages, resident by about 16 after the touch\n";
        return 0;
    }
    char* base = static_cast<char*>(p);
    report("after mmap 1 GiB  ", total0, resident0);

    touch(base);
    report("after 16 touches  ", total0, resident0);

    long sum = 0;
    for (std::size_t i = 0; i < 16; ++i) sum += base[i * 4096];
    std::cout << "  the 16 bytes read back as " << sum << " (every other byte of the GiB is still a promise)\n";

    munmap(base, kGiB);
    report("after munmap      ", total0, resident0);
    std::cout << "about 16 pages = 64 KiB of RAM for 1 GiB of address space; a page or two of noise is normal\n";
#else
    static char buffer[16 * 4096];
    touch(buffer);
    std::cout << "this sample needs Linux: it would mmap 1 GiB, touch 16 pages and read /proc/self/statm\n";
    std::cout << "expected there: total +262144 pages after the mmap, resident about +16 after the touch\n";
    std::cout << "here the 16 writes went to a static buffer (first byte " << static_cast<int>(buffer[0])
              << "): there is no /proc/self/statm to ask how much of it is resident\n";
#endif
    return 0;
}
