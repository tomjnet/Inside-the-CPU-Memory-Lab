// Build a Simple Memory Allocator: Understanding malloc and new - slide 3: the arena and the interface
// Build: make 03_arena_interface
#include <cstddef>
#include <cstdint>
#include <iostream>

// ---- slide 3: the arena and the interface ----
constexpr std::size_t ARENA = 4096;          // one page, fixed forever
constexpr std::size_t ALIGN = 16;            // what malloc promises
alignas(16) static unsigned char arena[ARENA];

// round n up to a multiple of 16: 1 -> 16, 16 -> 16, 17 -> 32
constexpr std::size_t align_up(std::size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

void* allocate(std::size_t size);            // nullptr when it is full
void  deallocate(void* ptr);                 // no size: we store it

// allocate() and deallocate() are only declared here: slide 4 defines the bump version,
// slides 6 and 8 the free list version. This program shows the arena and the rounding.

int main() {
    auto where = reinterpret_cast<std::uintptr_t>(arena);
    std::cout << "arena: " << ARENA << " bytes of static storage at " << static_cast<void*>(arena) << "\n";
    std::cout << "arena address % 16 = " << where % ALIGN << " (alignas(16) on the array)\n\n";

    std::cout << "align_up rounds every request to a multiple of " << ALIGN << ":\n";
    const std::size_t requests[] = {1, 15, 16, 17, 24, 100, 200, 1000};
    bool ok = where % ALIGN == 0;
    for (std::size_t n : requests) {
        std::size_t r = align_up(n);
        std::cout << "  align_up(" << n << ") = " << r << "  (" << r - n << " bytes of padding)\n";
        if (r % ALIGN != 0 || r < n || r - n >= ALIGN) ok = false;
    }
    std::cout << "\nalignof(std::max_align_t) with this compiler: " << alignof(std::max_align_t)
              << " (16 with g++ on x86 64; MSVC says 8, yet its malloc aligns to 16 too)\n";
    std::cout << (ok ? "every rounded size is a multiple of 16 and wastes less than 16 bytes\n"
                     : "ROUNDING IS WRONG\n");
    return ok ? 0 : 1;
}
