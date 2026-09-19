// CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 5: hit and miss: which line is my byte on?
// Build: make 05_line_of
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

constexpr std::uintptr_t kLine = 64;       // bytes per cache line
std::uintptr_t line_of(const void* p) {    // address / 64
    return reinterpret_cast<std::uintptr_t>(p) / kLine;
}

std::size_t lines_touched(const int* a, std::size_t n) {
    std::size_t lines = 1;                 // first touch: one miss
    for (std::size_t i = 1; i < n; ++i)    // new line: next miss
        if (line_of(&a[i]) != line_of(&a[i - 1])) ++lines;
    return lines;                          // 16 ints share a line
}

int main() {
    static int a[32] = {};
    std::cout << "int a[32]: " << sizeof(a) << " bytes, " << sizeof(int) << " bytes per int, "
              << kLine / sizeof(int) << " ints per 64 byte line\n\n";

    // the line number of a few elements: neighbours share it, then it changes by one
    const std::uintptr_t first = line_of(&a[0]);
    for (std::size_t i : {std::size_t{0}, std::size_t{3}, std::size_t{4}, std::size_t{15}, std::size_t{16},
                          std::size_t{31}}) {
        std::cout << "  a[" << i << "] at " << static_cast<const void*>(&a[i]) << "  line " << line_of(&a[i])
                  << "  (first line + " << line_of(&a[i]) - first << ")\n";
    }
    std::cout << "  the array does not have to start at the start of a line: a[0] is at byte "
              << reinterpret_cast<std::uintptr_t>(&a[0]) % kLine << " of its line\n\n";

    // how many lines a sequential walk touches: the upper limit on its misses
    bool ok = true;
    for (std::size_t n : {std::size_t{16}, std::size_t{1000}, std::size_t{1000000}}) {
        std::vector<int> v(n, 1);
        const std::size_t lines = lines_touched(v.data(), n);
        const std::size_t low = (n * sizeof(int) + kLine - 1) / kLine;   // array starts on a line boundary
        std::cout << "  " << n << " ints: " << lines << " cache lines touched, " << n << " reads, at most 1 miss per "
                  << static_cast<double>(n) / static_cast<double>(lines) << " reads\n";
        if (lines < low || lines > low + 1) ok = false;                 // one more when it starts mid line
    }

    std::cout << "\nthis is arithmetic, not a measurement: a program cannot ask the cache what it holds\n";
    if (!ok) {
        std::cout << "FAIL: the line count does not match bytes / 64\n";
        return 1;
    }
    std::cout << "check: every count is bytes / 64 rounded up, plus at most one\n";
    return 0;
}
