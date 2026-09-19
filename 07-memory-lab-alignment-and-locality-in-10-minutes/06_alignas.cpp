// Memory Alignment and Locality: Why Data Layout Matters - slide 6: alignas(64): one object per cache line
// Build: make 06_alignas
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

struct Counter {                  // 8 bytes: eight fit in one line
    std::uint64_t hits = 0;
};
struct alignas(64) LineCounter {  // 64 bytes: one per cache line
    std::uint64_t hits = 0;       // 8 bytes used, 56 of padding
};

template <class T>
static std::uintptr_t address_of(const T& x) { return reinterpret_cast<std::uintptr_t>(&x); }

int main() {
    static Counter     packed[4];     // 32 bytes: one cache line
    static LineCounter spread[4];     // 256 bytes: four cache lines

    std::cout << sizeof(Counter) << ' ' << alignof(Counter) << '\n';
    std::cout << sizeof(LineCounter) << ' '
              << alignof(LineCounter) << '\n';             // 64 64

    // Which cache line does each element start in? (address / 64, relative to element 0)
    std::cout << "\nelement   packed: line, offset in line   spread: line, offset in line\n";
    bool ok = true;
    for (int i = 0; i < 4; ++i) {
        packed[i].hits += 1;
        spread[i].hits += 1;
        std::uintptr_t p = address_of(packed[i]), s = address_of(spread[i]);
        std::cout << "   " << i << "              " << p / 64 - address_of(packed[0]) / 64 << ", " << p % 64
                  << "                       " << s / 64 - address_of(spread[0]) / 64 << ", " << s % 64 << "\n";
        if (s % 64 != 0) ok = false;               // every LineCounter must start on a line boundary
    }
    std::cout << "packed[4] takes " << sizeof(packed) << " bytes, spread[4] takes " << sizeof(spread) << " bytes\n";

    // Since C++17 new honours the extended alignment too.
    auto heap = std::make_unique<LineCounter>();
    heap->hits = packed[0].hits + spread[0].hits;
    std::cout << "heap LineCounter: address % 64 = " << address_of(*heap) % 64 << ", hits " << heap->hits << "\n";
    if (address_of(*heap) % 64 != 0) ok = false;

    std::cout << (ok ? "every LineCounter starts on a cache line boundary\n" : "ALIGNMENT BROKEN\n");
    return ok ? 0 : 1;
}
