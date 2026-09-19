// Stack vs Heap: Where Does Your Data Actually Live? - slide 9: lab: heap blocks and their hidden headers
// Build: make 09_heap_blocks
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#if defined(__linux__)
#include <malloc.h>
#endif

// signed distance in bytes from block a to block b (the blocks are unrelated arrays: compare as integers)
static long long gap(const void* a, const void* b) {
    return static_cast<long long>(reinterpret_cast<std::uintptr_t>(b))
         - static_cast<long long>(reinterpret_cast<std::uintptr_t>(a));
}

int main() {
    char* blocks[5];
    for (char*& b : blocks) {
        b = new char[24];                     // ask for 24 B, five times
    }
    std::cout << static_cast<void*>(blocks[0]) << "  first block\n";
    for (int i = 1; i < 5; ++i) {
        std::cout << static_cast<void*>(blocks[i]) << "  gap "
                  << gap(blocks[i - 1], blocks[i]) << " B\n";  // often 32
    }                                         // 24 B + header, 16 B aligned
    int on_stack = 0;                         // another region, far away
    std::cout << &on_stack << "  a local on the stack\n";
    std::cout << "distance from the first heap block to the local: "
              << gap(blocks[0], &on_stack) / (1024 * 1024) << " MiB (on_stack = " << on_stack << ")\n";
    for (char* b : blocks) delete[] b;        // every new[] needs delete[]

    std::cout << "\nWe asked for 24 B. A constant gap larger than 24 is the header plus the rounding\n";
    std::cout << "to the allocator's alignment (glibc: 32). Other allocators place blocks differently.\n\n";

#if defined(__linux__)
    std::cout << "what glibc really hands out (malloc_usable_size):\n";
    for (std::size_t request : {std::size_t{1}, std::size_t{24}, std::size_t{25}, std::size_t{100}}) {
        void* p = std::malloc(request);
        if (p == nullptr) return 1;
        std::cout << "  malloc(" << request << ") -> " << malloc_usable_size(p) << " usable bytes\n";
        std::free(p);
    }
#else
    std::cout << "this sample needs Linux: malloc_usable_size would show that malloc(1) and malloc(24)\n";
    std::cout << "both hand out 24 usable bytes inside a 32 byte block\n";
#endif
    return 0;
}
