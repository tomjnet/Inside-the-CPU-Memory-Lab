// Memory Addresses and Pointers: What Is Really Stored? - slide 9: nullptr: the address of nothing
// Build: make 09_nullptr
#include <cstddef>
#include <cstdint>
#include <iostream>

// nullptr: the pointer that points at no object
const int* find(const int* p, std::size_t n, int key) {
    for (std::size_t i = 0; i < n; ++i)
        if (p[i] == key) return p + i;     // address of the match
    return nullptr;                        // not found
}

int main() {
    int a[4] = {10, 20, 30, 40};
    const int* hit = find(a, 4, 30);
    if (hit) std::cout << hit - a << '\n';     // 2: test before the *
    const int* miss = find(a, 4, 99);
    std::cout << (miss == nullptr) << '\n';    // 1: never dereference

    // observed only, never dereferenced: the number inside a null pointer
    std::cout << "\nhit  = " << static_cast<const void*>(hit) << ", *hit = " << (hit ? *hit : -1) << "\n";
    std::cout << "miss as a number = " << reinterpret_cast<std::uintptr_t>(miss) << "\n";
    // std::cout << *miss;   // undefined behaviour: on Linux typically a segmentation fault, page 0 is never mapped
    std::cout << "sizeof(nullptr) = " << sizeof(nullptr) << ": it has its own type, std::nullptr_t\n";
    return (hit == a + 2 && miss == nullptr) ? 0 : 1;
}
